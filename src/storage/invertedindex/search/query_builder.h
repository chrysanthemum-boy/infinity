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

#include "query.h"
#include <stack>

import stl;

namespace infinity {

class QueryBuilder {
public:
    QueryBuilder();

    ~QueryBuilder();

    UniquePtr<QueryNode> Build();

    void AddTerm(const String &term, float weight = 1.0);

    void AddAnd(int child_count);

    void AddAndNot(int child_count);

    void AddOr(int child_count);

    void AddWAnd(int child_count);

    void AddPhrase(int child_count);

private:
    struct NodeInfo {
        MultiQuery *node_;
        int left_child_count_;
        NodeInfo(MultiQuery *node, int c) : node_(node), left_child_count_(c) {}
    };

    void AddTermQuery(QueryNode *n);

    void AddMultiQuery(MultiQuery *n, int child_count);

    UniquePtr<QueryNode> root_;
    std::stack<NodeInfo> nodes_;
};
} // namespace infinity