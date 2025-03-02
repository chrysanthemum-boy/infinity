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

#include <string>
#include <vector>

import stl;
import index_base;
import third_party;

import serialize;
import infinity_exception;
import statement_common;

module index_full_text;

namespace infinity {

void ToLowerString(String &lower) { std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower); }

SharedPtr<IndexBase> IndexFullText::Make(String file_name, Vector<String> column_names, const Vector<InitParameter *> &index_param_list) {
    String analyzer{};
    SizeT param_count = index_param_list.size();
    for (SizeT param_idx = 0; param_idx < param_count; ++param_idx) {
        InitParameter *parameter = index_param_list[param_idx];
        String para_name = parameter->param_name_;
        ToLowerString(para_name);
        if (para_name == "analyzer") {
            analyzer = parameter->param_value_;
        }
    }
    return MakeShared<IndexFullText>(file_name, std::move(column_names), analyzer);
}

bool IndexFullText::operator==(const IndexFullText &other) const {
    if (this->index_type_ != other.index_type_ || this->file_name_ != other.file_name_ || this->column_names_ != other.column_names_) {
        return false;
    }
    return analyzer_ == other.analyzer_;
}

bool IndexFullText::operator!=(const IndexFullText &other) const { return !(*this == other); }

i32 IndexFullText::GetSizeInBytes() const {
    SizeT size = IndexBase::GetSizeInBytes();
    size += sizeof(int32_t) + analyzer_.length();
    return size;
}

void IndexFullText::WriteAdv(char *&ptr) const {
    IndexBase::WriteAdv(ptr);
    WriteBufAdv(ptr, analyzer_);
}

SharedPtr<IndexBase> IndexFullText::ReadAdv(char *&, int32_t ) {
    UnrecoverableError("Not implemented");
    return nullptr;
}

String IndexFullText::ToString() const {
    std::stringstream ss;
    String output_str = IndexBase::ToString();
    if(!analyzer_.empty()) {
        output_str += ", " + analyzer_;
    }
    return output_str;
}

nlohmann::json IndexFullText::Serialize() const {
    nlohmann::json res = IndexBase::Serialize();
    res["analyzer"] = analyzer_;
    return res;
}

SharedPtr<IndexFullText> IndexFullText::Deserialize(const nlohmann::json &) {
    UnrecoverableError("Not implemented");
    return nullptr;
}

} // namespace infinity
