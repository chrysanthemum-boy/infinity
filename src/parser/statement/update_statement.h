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

#pragma once

#include "base_statement.h"
#include "expr/parsed_expr.h"
#include <vector>

namespace infinity {

struct UpdateExpr {
    inline ~UpdateExpr() {
        if (value != nullptr) {
            delete value;
            value = nullptr;
        }
    }

    std::string column_name;
    ParsedExpr *value;
};

class UpdateStatement final : public BaseStatement {
public:
    UpdateStatement() : BaseStatement(StatementType::kUpdate) {}

    ~UpdateStatement() final;

    [[nodiscard]] std::string ToString() const final;

    std::string schema_name_{"default"};
    std::string table_name_{};

    ParsedExpr *where_expr_{nullptr};

    std::vector<UpdateExpr *> *update_expr_array_{nullptr};
};

} // namespace infinity
