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

#include "unit_test/base_test.h"

import stl;
import infinity;
import query_result;
import data_block;
import value;
import query_options;

import database;
import table;
import logical_type;
import internal_types;
import parsed_expr;
import constant_expr;
import search_expr;
import column_expr;
import column_def;
import data_type;

class InfinityTest : public BaseTest {
    void SetUp() override {
        BaseTest::SetUp();
        system("rm -rf /tmp/infinity/log /tmp/infinity/data /tmp/infinity/wal");
    }
    void TearDown() override {
        system("rm -rf /tmp/infinity/log /tmp/infinity/data /tmp/infinity/wal");
        BaseTest::TearDown();
    }
};

TEST_F(InfinityTest, test1) {
    using namespace infinity;
    String path = "/tmp/infinity";

    Infinity::LocalInit(path);

    SharedPtr<Infinity> infinity = Infinity::LocalConnect();

    {
        QueryResult result = infinity->ListDatabases();
        //    EXPECT_EQ(result.result_table_->row_count(), 1); // Bug
        EXPECT_EQ(result.result_table_->ColumnCount(), 1u);
        EXPECT_EQ(result.result_table_->GetColumnNameById(0), "database");
        EXPECT_EQ(result.result_table_->DataBlockCount(), 1u);
        SharedPtr<DataBlock> data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 1u);
        Value value = data_block->GetValue(0, 0);
        const String &s2 = value.GetVarchar();
        EXPECT_STREQ(s2.c_str(), "default");
    }

    {
        CreateDatabaseOptions create_db_opts;
        infinity->CreateDatabase("db1", create_db_opts);
        QueryResult result = infinity->ListDatabases();
        SharedPtr<DataBlock> data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 2);
        Value value = data_block->GetValue(0, 0);
        const String &s2 = value.GetVarchar();
        EXPECT_STREQ(s2.c_str(), "db1");

        value = data_block->GetValue(0, 1);
        const String &s3 = value.GetVarchar();
        EXPECT_STREQ(s3.c_str(), "default");

        SharedPtr<Database> db1_ptr = infinity->GetDatabase("db1");
        EXPECT_EQ(db1_ptr->db_name(), "db1");

        SharedPtr<Database> db2_ptr = infinity->GetDatabase("db2");
        EXPECT_EQ(db2_ptr, nullptr);

        DropDatabaseOptions drop_db_opts;
        result = infinity->DropDatabase("db1", drop_db_opts);
        EXPECT_FALSE(result.IsOk());

        SharedPtr<Database> default_db_ptr = infinity->GetDatabase("default");
        EXPECT_EQ(default_db_ptr->db_name(), "default");

        result = infinity->DropDatabase("db1", drop_db_opts);
        EXPECT_TRUE(result.IsOk());

        result = infinity->ListDatabases();
        data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 1);
        value = data_block->GetValue(0, 0);
        const String &s4 = value.GetVarchar();
        EXPECT_STREQ(s4.c_str(), "default");
        SharedPtr<Database> db_instance = infinity->GetDatabase("default");
    }

    {
        SharedPtr<Database> db1_ptr = infinity->GetDatabase("default");
        EXPECT_EQ(db1_ptr->db_name(), "default");

        CreateTableOptions create_table_opts;

        SizeT column_count = 2;
        //        Vector<SharedPtr<ColumnDef>> columns;
        Vector<ColumnDef *> column_defs;
        column_defs.reserve(column_count);

        SharedPtr<DataType> col_type = MakeShared<DataType>(LogicalType::kBoolean);
        String col_name = "col1";
        auto col_def = new ColumnDef(0, col_type, col_name, HashSet<ConstraintType>());
        column_defs.emplace_back(col_def);

        col_type = MakeShared<DataType>(LogicalType::kBigInt);
        col_name = "col2";
        col_def = new ColumnDef(1, col_type, col_name, HashSet<ConstraintType>());
        column_defs.emplace_back(col_def);

        QueryResult result = db1_ptr->CreateTable("table1", column_defs, Vector<TableConstraint *>(), create_table_opts);
        EXPECT_TRUE(result.IsOk());

        result = db1_ptr->ListTables();
        SharedPtr<DataBlock> data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 1);
        Value value = data_block->GetValue(1, 0);
        const String &s2 = value.GetVarchar();
        EXPECT_STREQ(s2.c_str(), "table1");

        SharedPtr<Table> table1 = db1_ptr->GetTable("table1");
        EXPECT_NE(table1, nullptr);

        DropTableOptions drop_table_opts;
        result = db1_ptr->DropTable("table1", drop_table_opts);
        EXPECT_TRUE(result.IsOk());
        result = db1_ptr->ListTables();
        data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 0);

        table1 = db1_ptr->GetTable("table1");
        EXPECT_EQ(table1, nullptr);
    }

    {
        SharedPtr<Database> db1_ptr = infinity->GetDatabase("default");
        EXPECT_EQ(db1_ptr->db_name(), "default");

        CreateTableOptions create_table_opts;

        SizeT column_count = 1;
        Vector<ColumnDef *> column_defs;
        column_defs.reserve(column_count);

        SharedPtr<DataType> col_type = MakeShared<DataType>(LogicalType::kBigInt);
        String col1_name = "col1";
        auto col_def = new ColumnDef(0, col_type, col1_name, HashSet<ConstraintType>());
        column_defs.emplace_back(col_def);

        col_type = MakeShared<DataType>(LogicalType::kSmallInt);
        String col2_name = "col2";
        col_def = new ColumnDef(1, col_type, col2_name, HashSet<ConstraintType>());
        column_defs.emplace_back(col_def);

        QueryResult result = db1_ptr->CreateTable("table1", column_defs, Vector<TableConstraint *>(), create_table_opts);
        EXPECT_TRUE(result.IsOk());

        SharedPtr<Table> table1 = db1_ptr->GetTable("table1");
        EXPECT_NE(table1, nullptr);

        //        Vector<String> *columns, Vector<Vector<ParsedExpr *> *> *values

        Vector<String> *columns = new Vector<String>();
        columns->emplace_back(col1_name);
        columns->emplace_back(col2_name);

        Vector<Vector<ParsedExpr *> *> *values = new Vector<Vector<ParsedExpr *> *>();
        values->emplace_back(new Vector<ParsedExpr *>());

        ConstantExpr *value1 = new ConstantExpr(LiteralType::kInteger);
        value1->integer_value_ = 11;
        values->at(0)->emplace_back(value1);

        ConstantExpr *value2 = new ConstantExpr(LiteralType::kInteger);
        value2->integer_value_ = 22;
        values->at(0)->emplace_back(value2);
        table1->Insert(columns, values);

        //        QueryResult Search(Vector<Pair<ParsedExpr *, ParsedExpr *>> &vector_expr,
        //                           Vector<Pair<ParsedExpr *, ParsedExpr *>> &fts_expr,
        //                           ParsedExpr *filter,
        //                           Vector<ParsedExpr *> *output_columns,
        //                           ParsedExpr *offset,
        //                           ParsedExpr *limit);

        Vector<ParsedExpr *> *output_columns = new Vector<ParsedExpr *>();
        ColumnExpr *col1 = new ColumnExpr();
        col1->names_.emplace_back(col1_name);
        output_columns->emplace_back(col1);

        ColumnExpr *col2 = new ColumnExpr();
        col2->names_.emplace_back(col2_name);
        output_columns->emplace_back(col2);

        SearchExpr * search_expr = nullptr;

        result = table1->Search(search_expr, nullptr, output_columns);
        SharedPtr<DataBlock> data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 1);
        Value value = data_block->GetValue(0, 0);
        EXPECT_EQ(value.type().type(), LogicalType::kBigInt);
        EXPECT_EQ(value.value_.big_int, 11);

        value = data_block->GetValue(1, 0);
        EXPECT_EQ(value.type().type(), LogicalType::kSmallInt);
        EXPECT_EQ(value.value_.big_int, 22);

        DropTableOptions drop_table_opts;
        result = db1_ptr->DropTable("table1", drop_table_opts);
        EXPECT_TRUE(result.IsOk());
    }

    {
        infinity->Query("create database db1;");
        QueryResult result = infinity->Query("show databases;");
        SharedPtr<DataBlock> data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 2);
        Value value = data_block->GetValue(0, 0);
        const String &s2 = value.GetVarchar();
        EXPECT_STREQ(s2.c_str(), "db1");

        value = data_block->GetValue(0, 1);
        const String &s3 = value.GetVarchar();
        EXPECT_STREQ(s3.c_str(), "default");

        result = infinity->Query("drop database db1");
        EXPECT_TRUE(result.IsOk());

        result = infinity->ListDatabases();
        data_block = result.result_table_->GetDataBlockById(0);
        EXPECT_EQ(data_block->row_count(), 1);
        value = data_block->GetValue(0, 0);
        const String &s4 = value.GetVarchar();
        EXPECT_STREQ(s4.c_str(), "default");
        SharedPtr<Database> db_instance = infinity->GetDatabase("default");
    }
    infinity->LocalDisconnect();

    Infinity::LocalUnInit();
}

TEST_F(InfinityTest, test2) {
    using namespace infinity;
    String path = "/tmp/infinity";

    Infinity::LocalInit(path);

    SharedPtr<Infinity> infinity = Infinity::LocalConnect();

    {
        QueryResult result = infinity->ShowVariable("version");
        EXPECT_EQ(result.IsOk(), true);
    }

    {
        QueryResult result = infinity->ShowVariable("session_count");
        EXPECT_EQ(result.IsOk(), true);
    }

    {
        QueryResult result = infinity->ShowVariable("query_count");
        EXPECT_EQ(result.IsOk(), true);
    }

    {
        QueryResult result = infinity->ShowVariable("buffer_pool_usage");
        EXPECT_EQ(result.IsOk(), true);
    }

    {
        QueryResult result = infinity->ShowVariable("error");
        EXPECT_EQ(result.IsOk(), false);
    }

    infinity->LocalDisconnect();

    Infinity::LocalUnInit();
}