//
// Created by JinHai on 2022/9/11.
//

#include "binding.h"
#include "common/utility/infinity_assert.h"

namespace infinity {

std::shared_ptr<Binding>
Binding::MakeBinding(BindingType binding_type, const std::string& name,
                     const std::vector<LogicalType>& column_types, const std::vector<std::string>& column_names) {
    std::shared_ptr<Binding> binding = std::make_shared<Binding>();
    binding->binding_type_ = binding_type;
    binding->table_name_ = name;

    size_t column_count = column_names.size();
    PlannerAssert(column_count == column_types.size(), "Make binding error: column size isn't valid.");
    binding->column_types_ = column_types;
    binding->column_names_ = column_names;

    for(size_t idx = 0; idx < column_count; ++ idx) {
        binding->name2index_[column_names[idx]] = static_cast<int64_t>(idx);
    }
    return binding;
}

std::shared_ptr<Binding>
Binding::MakeBinding(BindingType binding_type, const std::string& name, std::shared_ptr<Table> table_ptr,
            const std::vector<LogicalType>& column_types, const std::vector<std::string>& column_names) {
    auto binding = MakeBinding(binding_type, name, column_types, column_names);
    binding->table_ptr_ = std::move(table_ptr);
//    binding->logical_node_ptr_ = std::move(logical_node_ptr);
//    binding->logical_node_id_ = logical_node_id;
    return binding;
}

}