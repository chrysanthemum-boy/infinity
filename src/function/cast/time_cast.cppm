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

export module time_cast;

import stl;
import column_vector_cast;
import logical_type;
import infinity_exception;
import bound_cast_func;
import column_vector;
import third_party;
import internal_types;
import data_type;

namespace infinity {

export struct TimeTryCastToVarlen;

export inline BoundCastFunc BindTimeCast(DataType &target) {
    switch (target.type()) {
        case LogicalType::kVarchar: {
            return BoundCastFunc(&ColumnVectorCast::TryCastColumnVectorToVarlen<TimeT, VarcharT, TimeTryCastToVarlen>);
        }
        default: {
            UnrecoverableError(
                    fmt::format("Can't cast from Time type to  {}", target.ToString()));
        }
    }
    return BoundCastFunc(nullptr);
}

struct TimeTryCastToVarlen {
    template <typename SourceType, typename TargetType>
    static inline bool Run(SourceType, TargetType &, const SharedPtr<ColumnVector> &) {
        UnrecoverableError(
                fmt::format("Not support to cast from {} to {}", DataType::TypeToString<SourceType>(), DataType::TypeToString<TargetType>()));
        return false;
    }
};

template <>
inline bool TimeTryCastToVarlen::Run(TimeT, VarcharT &, const SharedPtr<ColumnVector> &) {
    UnrecoverableError("Not implement: IntegerTryCastToFixlen::Run");
    return false;
}

} // namespace infinity
