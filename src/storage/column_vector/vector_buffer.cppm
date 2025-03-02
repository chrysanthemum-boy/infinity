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

export module vector_buffer;

import stl;
import global_resource_usage;
import heap_chunk;
import fix_heap;
import buffer_handle;

namespace infinity {

class BufferManager;
class BlockColumnEntry;

export enum class VectorBufferType { kInvalid, kStandard, kHeap, kCompactBit };

export class VectorBuffer {
public:
    static SharedPtr<VectorBuffer> Make(SizeT data_type_size, SizeT capacity, VectorBufferType buffer_type);

    static SharedPtr<VectorBuffer>
    Make(BufferManager *buffer_mgr, BlockColumnEntry *block_column_entry, SizeT data_type_size, SizeT capacity, VectorBufferType buffer_type);

public:
    explicit VectorBuffer() { GlobalResourceUsage::IncrObjectCount(); }

    ~VectorBuffer() { GlobalResourceUsage::DecrObjectCount(); }

    void Initialize(SizeT type_size, SizeT capacity);

    void InitializeCompactBit(SizeT capacity);

    void InitializeCompactBit(BufferManager *buffer_mgr, BlockColumnEntry *block_column_entry, SizeT capacity);

    void Initialize(BufferManager *buffer_mgr, BlockColumnEntry *block_column_entry, SizeT type_size, SizeT capacity);

    void ResetToInit();

    void Copy(ptr_t input, SizeT size);

    [[nodiscard]] ptr_t GetDataMut() {
        if (std::holds_alternative<UniquePtr<char[]>>(ptr_)) {
            return std::get<UniquePtr<char[]>>(ptr_).get();
        } else {
            return static_cast<ptr_t>(std::get<BufferHandle>(ptr_).GetDataMut());
        }
    }

    [[nodiscard]] const_ptr_t GetData() const {
        if (std::holds_alternative<UniquePtr<char[]>>(ptr_)) {
            return std::get<UniquePtr<char[]>>(ptr_).get();
        } else {
            return static_cast<const_ptr_t>(std::get<BufferHandle>(ptr_).GetData());
        }
    }

    [[nodiscard]] bool GetCompactBit(SizeT idx) const;

    void SetCompactBit(SizeT idx, bool val);

    [[nodiscard]] static bool RawPointerGetCompactBit(const u8 *src_ptr_u8, SizeT idx);

    static void RawPointerSetCompactBit(u8 *dst_ptr_u8, SizeT idx, bool val);

    static bool CompactBitIsSame(const SharedPtr<VectorBuffer> &lhs, SizeT lhs_cnt, const SharedPtr<VectorBuffer> &rhs, SizeT rhs_cnt);

    static void CopyCompactBits(u8 *dst, const u8 *src, SizeT dst_start_id, SizeT src_start_id, SizeT count);

private:
    bool initialized_{false};

    std::variant<UniquePtr<char[]>, BufferHandle> ptr_;

    SizeT data_size_{0};
    SizeT capacity_{0};

public:
    VectorBufferType buffer_type_{VectorBufferType::kInvalid};

    UniquePtr<FixHeapManager> fix_heap_mgr_{nullptr};
};

} // namespace infinity
