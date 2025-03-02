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

module block_index;

import stl;
import segment_entry;
import global_block_id;
import block_iter;
import segment_iter;

namespace infinity {

void BlockIndex::Insert(SegmentEntry *segment_entry, TxnTimeStamp timestamp, bool check_ts) {
    if (!check_ts || (timestamp >= segment_entry->min_row_ts() && timestamp <= segment_entry->deprecate_ts())) {
        u32 segment_id = segment_entry->segment_id();
        segments_.emplace_back(segment_entry);
        segment_index_.emplace(segment_id, segment_entry);

        auto block_entry_iter = BlockEntryIter(segment_entry);
        for (auto *block_entry = block_entry_iter.Next(); block_entry != nullptr; block_entry = block_entry_iter.Next()) {
            if (timestamp >= block_entry->min_row_ts()) {
                segment_block_index_[segment_id].emplace(block_entry->block_id(), block_entry);
                global_blocks_.emplace_back(GlobalBlockID{segment_id, block_entry->block_id()});
            }
        }
    }
}

void BlockIndex::Reserve(SizeT n) {
    segments_.reserve(n);
    segment_index_.reserve(n);
    segment_block_index_.reserve(n);
}

BlockEntry *BlockIndex::GetBlockEntry(u32 segment_id, u16 block_id) const {
    auto seg_it = segment_block_index_.find(segment_id);
    if (seg_it != segment_block_index_.end()) {
        auto block_it = seg_it->second.find(block_id);
        if (block_it != seg_it->second.end()) {
            return block_it->second;
        }
    }
    return nullptr;
}

} // namespace infinity
