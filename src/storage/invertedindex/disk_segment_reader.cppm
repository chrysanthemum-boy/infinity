// Copyright(C) 2023 InfiniFlow, Inc. All rights reserved.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

module;

export module disk_index_segment_reader;
import stl;
import memory_pool;
import segment_posting;
import index_defines;
import index_segment_reader;
import index_config;
import segment;
import dict_reader;
import file_reader;
import posting_list_format;
import local_file_system;

namespace infinity {
export class DiskIndexSegmentReader : public IndexSegmentReader {
public:
    DiskIndexSegmentReader(u64 column_id, const Segment &segment, const InvertedIndexConfig &index_config);
    virtual ~DiskIndexSegmentReader();

    bool GetSegmentPosting(const String &term, docid_t base_doc_id, SegmentPosting &seg_posting, MemoryPool *session_pool) const override;

private:
    u64 column_id_;
    const Segment &segment_;
    SharedPtr<DictionaryReader> dict_reader_;
    SharedPtr<FileReader> posting_reader_;
    docid_t base_doc_id_{INVALID_DOCID};
    PostingFormatOption posting_format_option_;
    LocalFileSystem fs_;
};

} // namespace infinity