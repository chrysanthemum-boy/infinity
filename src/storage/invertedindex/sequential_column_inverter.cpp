module;

#include <arpa/inet.h>
#include <cstring>
#include <vector>

module sequential_column_inverter;
import stl;
import analyzer;
import memory_pool;
import pool_allocator;
import string_ref;
import term;
import radix_sort;
import index_defines;
import memory_indexer;
namespace infinity {

template <u32 T>
static u32 Align(u32 unaligned) {
    return (unaligned + T - 1) & (-T);
}

SequentialColumnInverter::SequentialColumnInverter(MemoryIndexer *memory_indexer)
    : memory_indexer_(memory_indexer), analyzer_(memory_indexer->GetAnalyzer()), jieba_specialize_(memory_indexer->IsJiebaSpecialize()),
      alloc_(memory_indexer->GetPool()), terms_(alloc_), positions_(alloc_), term_refs_(alloc_) {}

SequentialColumnInverter::~SequentialColumnInverter() {}

bool SequentialColumnInverter::CompareTermRef::operator()(const u32 lhs, const u32 rhs) const { return std::strcmp(GetTerm(lhs), GetTerm(rhs)) < 0; }

void SequentialColumnInverter::InvertColumn(SharedPtr<ColumnVector> column_vector, RowID start_row_id) {
    docid_t start_doc_id = RowID2DocID(start_row_id);
    for (SizeT i = 0; i < column_vector->Size(); ++i) {
        String data = column_vector->ToString(i);
        InvertColumn(start_doc_id + i, data);
    }
}

void SequentialColumnInverter::InvertColumn(u32 doc_id, const String &val) {
    terms_once_.clear();
    analyzer_->Analyze(val, terms_once_, jieba_specialize_);
    for (auto it = terms_once_.begin(); it != terms_once_.end(); ++it) {
        StringRef term(it->text_);
        u32 term_ref = AddTerm(term);
        positions_.emplace_back(term_ref, doc_id, it->word_offset_);
    }
}

u32 SequentialColumnInverter::AddTerm(StringRef term) {
    const u32 terms_size = terms_.size();
    const u32 unpadded_size = terms_size + 4 + term.size() + 1;
    const u32 fully_padded_size = Align<4>(unpadded_size);
    terms_.resize(fully_padded_size);

    char *buf = &terms_[0] + terms_size;
    memset(buf, 0, 4);
    memcpy(buf + 4, term.data(), term.size());
    memset(buf + 4 + term.size(), 0, fully_padded_size - unpadded_size + 1);

    u32 term_ref = (terms_size + 4) >> 2;
    term_refs_.push_back(term_ref);
    return term_ref;
}

struct TermRefRadix {
    u32 operator()(const u64 v) { return v >> 32; }
};

void SequentialColumnInverter::SortTerms() {
    Vector<u64> first_four_bytes(term_refs_.size());
    for (u32 i = 1; i < term_refs_.size(); ++i) {
        u64 first_four = ntohl(*reinterpret_cast<const u32 *>(GetTermFromRef(term_refs_[i])));
        first_four_bytes[i] = (first_four << 32) | term_refs_[i];
    }
    ShiftBasedRadixSorter<u64, TermRefRadix, CompareTermRef, 24, true>::RadixSort(TermRefRadix(),
                                                                                  CompareTermRef(terms_),
                                                                                  &first_four_bytes[1],
                                                                                  first_four_bytes.size() - 1,
                                                                                  16);
    for (u32 i(1); i < first_four_bytes.size(); i++) {
        term_refs_[i] = first_four_bytes[i] & 0xffffffffl;
    }
    auto term_ref_begin(term_refs_.begin() + 1);
    uint32_t term_num = 1; // First valid term number
    const char *last_term = GetTermFromRef(*term_ref_begin);
    UpdateTermNum(*term_ref_begin, term_num);
    for (++term_ref_begin; term_ref_begin != term_refs_.end(); ++term_ref_begin) {
        const char *term = GetTermFromRef(*term_ref_begin);
        int cmpres = strcmp(last_term, term);
        // assert(cmpres <= 0);
        if (cmpres < 0) {
            ++term_num;
            term_refs_[term_num] = *term_ref_begin;
            last_term = term;
        }
        UpdateTermNum(*term_ref_begin, term_num);
    }
    // assert(term_refs_.size() >= term_num + 1);
    term_refs_.resize(term_num + 1);
    // Replace initial word reference by word number.
    for (auto &p : positions_) {
        p.term_num_ = GetTermNum(p.term_num_);
    }
}

struct FullRadix {
    u64 operator()(const SequentialColumnInverter::PosInfo &p) const { return (static_cast<u64>(p.term_num_) << 32) | p.doc_id_; }
};

void SequentialColumnInverter::Commit() {
    SortTerms();
    ShiftBasedRadixSorter<PosInfo, FullRadix, std::less<PosInfo>, 56, true>::RadixSort(FullRadix(),
                                                                                       std::less<PosInfo>(),
                                                                                       &positions_[0],
                                                                                       positions_.size(),
                                                                                       16);
    if (memory_indexer_->IsRealTime()) {
        DoRTInsert();
    } else {
        DoInsert();
    }
    memory_indexer_->TryDump();
}

void SequentialColumnInverter::DoInsert() {
    u32 last_term_num = 0;
    u32 last_term_pos = 0;
    u32 last_doc_id = 0;
    StringRef term;
    MemoryIndexer::PostingPtr posting = nullptr;
    for (auto &i : positions_) {
        if (last_term_num != i.term_num_ || last_doc_id != i.doc_id_) {
            if (last_term_num != i.term_num_) {
                last_term_num = i.term_num_;
                term = GetTermFromNum(last_term_num);
                posting = memory_indexer_->GetOrAddPosting(String(term.data()));
            }
            last_doc_id = i.doc_id_;
            if (last_doc_id != 0) {
                posting->EndDocument(last_doc_id, 0);
            }
        }
        if (i.term_pos_ != last_term_pos) {
            last_term_pos = i.term_pos_;
            posting->AddPosition(last_term_pos);
        }
    }
}

void SequentialColumnInverter::DoRTInsert() {
    u32 last_term_num = 0;
    u32 last_term_pos = 0;
    u32 last_doc_id = 0;
    StringRef term;
    MemoryIndexer::RTPostingPtr posting = nullptr;
    for (auto &i : positions_) {
        if (last_term_num != i.term_num_ || last_doc_id != i.doc_id_) {
            if (last_term_num != i.term_num_) {
                last_term_num = i.term_num_;
                term = GetTermFromNum(last_term_num);
                posting = memory_indexer_->GetOrAddRTPosting(String(term.data()));
            }
            last_doc_id = i.doc_id_;
            if (last_doc_id != 0) {
                posting->EndDocument(last_doc_id, 0);
            }
        }
        if (i.term_pos_ != last_term_pos) {
            last_term_pos = i.term_pos_;
            posting->AddPosition(last_term_pos);
        }
    }
}

} // namespace infinity