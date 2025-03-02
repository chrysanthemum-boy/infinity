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

#include "type/complex/embedding_type.h"
#include "unit_test/base_test.h"
#include <memory>

import stl;
import global_resource_usage;
import storage;
import infinity_context;

import txn_manager;
import table_def;
import data_block;
import value;
import txn_store;
import buffer_manager;
import meta_state;
import wal_entry;
import infinity_exception;
import status;
import column_vector;
import physical_import;
import txn;
import catalog;
import index_base;
import index_ivfflat;
import index_hnsw;
import index_base;
import index_full_text;
import index_def;
import bg_task;
import backgroud_process;
import compact_segments_task;
import default_values;
import base_table_ref;
import internal_types;
import logical_type;
import embedding_info;
import extra_ddl_info;
import knn_expr;
import column_def;
import statement_common;
import data_type;

import segment_entry;
import block_entry;
import block_column_entry;
import table_index_entry;
import base_entry;

class WalReplayTest : public BaseTest {
    void SetUp() override { system("rm -rf /tmp/infinity/log /tmp/infinity/data /tmp/infinity/wal"); }

    void TearDown() override {
        system("tree  /tmp/infinity");
        system("rm -rf /tmp/infinity/log /tmp/infinity/data /tmp/infinity/wal");
    }
};

using namespace infinity;

TEST_F(WalReplayTest, WalReplayDatabase) {
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BGTaskProcessor *bg_processor = storage->bg_processor();

        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->CreateDatabase("db1", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->CreateDatabase("db2", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->CreateDatabase("db3", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->CreateDatabase("db4", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            SharedPtr<ForceCheckpointTask> force_ckp_task = MakeShared<ForceCheckpointTask>(txn, false);
            bg_processor->Submit(force_ckp_task);
            force_ckp_task->Wait();
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->CreateDatabase("db5", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            txn->DropDatabase("db1", ConflictType::kIgnore);
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }

    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->DropDatabase("db4", ConflictType::kError);
            EXPECT_EQ(status.ok(), true);
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateDatabase("db1", ConflictType::kError);
            EXPECT_EQ(status.ok(), true);
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayTables) {

    Vector<SharedPtr<ColumnDef>> columns;
    {
        i64 column_id = 0;
        {
            HashSet<ConstraintType> constraints;
            constraints.insert(ConstraintType::kUnique);
            constraints.insert(ConstraintType::kNotNull);
            auto column_def_ptr =
                MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kTinyInt)), "tiny_int_col", constraints);
            columns.emplace_back(column_def_ptr);
        }
        {
            HashSet<ConstraintType> constraints;
            constraints.insert(ConstraintType::kPrimaryKey);
            auto column_def_ptr =
                MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kBigInt)), "big_int_col", constraints);
            columns.emplace_back(column_def_ptr);
        }
        {
            HashSet<ConstraintType> constraints;
            constraints.insert(ConstraintType::kNotNull);
            auto column_def_ptr = MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kDouble)), "double_col", constraints);
            columns.emplace_back(column_def_ptr);
        }
    }
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BGTaskProcessor *bg_processor = storage->bg_processor();

        {
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl1"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto tbl2_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl2"), columns);
            auto *txn2 = txn_mgr->CreateTxn();
            txn2->Begin();
            Status status2 = txn2->CreateTable("default", std::move(tbl2_def), ConflictType::kIgnore);
            EXPECT_TRUE(status2.ok());
            txn_mgr->CommitTxn(txn2);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->DropTableCollectionByName("default", "tbl2", ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto tbl3_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl3"), columns);
            auto *txn3 = txn_mgr->CreateTxn();
            txn3->Begin();
            Status status = txn3->CreateTable("default", std::move(tbl3_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn3);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            SharedPtr<ForceCheckpointTask> force_ckp_task = MakeShared<ForceCheckpointTask>(txn, false);
            bg_processor->Submit(force_ckp_task);
            force_ckp_task->Wait();
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        {
            auto tbl2_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl2"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl2_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->DropTableCollectionByName("default", "tbl3", ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayAppend) {
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BGTaskProcessor *bg_processor = storage->bg_processor();

        Vector<SharedPtr<ColumnDef>> columns;
        {
            i64 column_id = 0;
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kUnique);
                constraints.insert(ConstraintType::kNotNull);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kTinyInt)), "tiny_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kPrimaryKey);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kBigInt)), "big_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kNotNull);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kDouble)), "double_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
        }

        {
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl1"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto tbl3_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl3"), columns);
            auto *txn3 = txn_mgr->CreateTxn();
            txn3->Begin();
            Status status = txn3->CreateTable("default", std::move(tbl3_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn3);
        }
        {
            auto tbl4_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl4"), columns);
            auto *txn4 = txn_mgr->CreateTxn();
            txn4->Begin();
            Status status = txn4->CreateTable("default", std::move(tbl4_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn4);
        }
        {
            auto *txn5 = txn_mgr->CreateTxn();
            SharedPtr<DataBlock> input_block = MakeShared<DataBlock>();
            SizeT row_count = 2;

            // Prepare the input data block
            Vector<SharedPtr<DataType>> column_types;
            column_types.emplace_back(MakeShared<DataType>(LogicalType::kTinyInt));
            column_types.emplace_back(MakeShared<DataType>(LogicalType::kBigInt));
            column_types.emplace_back(MakeShared<DataType>(LogicalType::kDouble));

            input_block->Init(column_types, row_count);
            for (SizeT i = 0; i < row_count; ++i) {
                input_block->AppendValue(0, Value::MakeTinyInt(static_cast<i8>(i)));
            }

            for (SizeT i = 0; i < row_count; ++i) {
                input_block->AppendValue(1, Value::MakeBigInt(static_cast<i64>(i)));
            }

            for (SizeT i = 0; i < row_count; ++i) {
                input_block->AppendValue(2, Value::MakeDouble(static_cast<f64>(i)));
            }
            input_block->Finalize();
            EXPECT_EQ(input_block->Finalized(), true);
            txn5->Begin();
            txn5->Append("default", "tbl4", input_block);
            txn_mgr->CommitTxn(txn5);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            SharedPtr<ForceCheckpointTask> force_ckp_task = MakeShared<ForceCheckpointTask>(txn, false);
            bg_processor->Submit(force_ckp_task);
            force_ckp_task->Wait();
            txn_mgr->CommitTxn(txn);
        }
        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
    // Restart the db instance
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        Vector<SharedPtr<ColumnDef>> columns;
        {
            i64 column_id = 0;
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kUnique);
                constraints.insert(ConstraintType::kNotNull);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kTinyInt)), "tiny_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
        }
        {
            auto tbl5_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl5"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();

            Status status = txn->CreateTable("default", std::move(tbl5_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());

            txn_mgr->CommitTxn(txn);
        }
        //        {
        //            auto *txn = txn_mgr->CreateTxn();
        //            txn->Begin();
        //            Vector<ColumnID> column_ids{0, 1, 2};
        //            UniquePtr<MetaTableState> read_table_meta = MakeUnique<MetaTableState>();
        //
        //            txn->GetMetaTableState(read_table_meta.get(), "default", "tbl4", column_ids);
        //            EXPECT_EQ(read_table_meta->segment_map_.size(), 1);
        //
        //            EXPECT_EQ(read_table_meta->local_blocks_.size(), 0);
        //            EXPECT_EQ(read_table_meta->segment_map_.size(), 1);
        //            for (const auto &segment_pair : read_table_meta->segment_map_) {
        //                EXPECT_EQ(segment_pair.first, 0);
        //                EXPECT_NE(segment_pair.second.segment_entry_, nullptr);
        //                EXPECT_EQ(segment_pair.second.segment_entry_->block_entries_.size(), 1);
        //                EXPECT_EQ(segment_pair.second.block_map_.size(), 1);
        //                for (const auto &block_pair : segment_pair.second.block_map_) {
        //                    //                    EXPECT_EQ(block_pair.first, 0);
        //                    EXPECT_NE(block_pair.second.block_entry_, nullptr);
        //
        //                    EXPECT_EQ(block_pair.second.column_data_map_.size(), 3);
        //                    EXPECT_TRUE(block_pair.second.column_data_map_.contains(0));
        //                    EXPECT_TRUE(block_pair.second.column_data_map_.contains(1));
        //                    EXPECT_TRUE(block_pair.second.column_data_map_.contains(2));
        //
        //                    BlockColumnEntry *column0 = block_pair.second.column_data_map_.at(0).block_column_;
        //                    BlockColumnEntry *column1 = block_pair.second.column_data_map_.at(1).block_column_;
        //                    BlockColumnEntry *column2 = block_pair.second.column_data_map_.at(2).block_column_;
        //
        //                    SizeT row_count = block_pair.second.block_entry_->row_count_;
        //                    ColumnBuffer col0_obj = BlockColumnEntry::GetColumnData(column0, buffer_manager);
        //                    i8 *col0_ptr = (i8 *)(col0_obj.GetAll());
        //                    for (SizeT row = 0; row < row_count; ++row) {
        //                        EXPECT_EQ(col0_ptr[row], (i8)(row));
        //                    }
        //
        //                    ColumnBuffer col1_obj = BlockColumnEntry::GetColumnData(column1, buffer_manager);
        //                    i64 *col1_ptr = (i64 *)(col1_obj.GetAll());
        //                    for (SizeT row = 0; row < row_count; ++row) {
        //                        EXPECT_EQ(col1_ptr[row], (i64)(row));
        //                    }
        //
        //                    ColumnBuffer col2_obj = BlockColumnEntry::GetColumnData(column2, buffer_manager);
        //                    f64 *col2_ptr = (f64 *)(col2_obj.GetAll());
        //                    for (SizeT row = 0; row < row_count; ++row) {
        //                        EXPECT_FLOAT_EQ(col2_ptr[row], row % 8192);
        //                    }
        //                }
        //            }
        //        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayImport) {
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BufferManager *buffer_manager = storage->buffer_manager();
        BGTaskProcessor *bg_processor = storage->bg_processor();

        Vector<SharedPtr<ColumnDef>> columns;
        {
            i64 column_id = 0;
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kUnique);
                constraints.insert(ConstraintType::kNotNull);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kTinyInt)), "tiny_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kPrimaryKey);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kBigInt)), "big_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kNotNull);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kDouble)), "double_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
        }
        int column_count = columns.size();

        {
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl1"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto tbl2_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl2"), columns);
            auto *txn2 = txn_mgr->CreateTxn();
            txn2->Begin();
            Status status = txn2->CreateTable("default", std::move(tbl2_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn2);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            SharedPtr<ForceCheckpointTask> force_ckp_task = MakeShared<ForceCheckpointTask>(txn, false);
            bg_processor->Submit(force_ckp_task);
            force_ckp_task->Wait();
            txn_mgr->CommitTxn(txn);
        }
        {
            auto *txn = txn_mgr->CreateTxn();
            auto tbl3_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl3"), columns);
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl3_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        {
            auto txn4 = txn_mgr->CreateTxn();
            txn4->Begin();

            auto [table_entry, status] = txn4->GetTableEntry("default", "tbl1");
            EXPECT_NE(table_entry, nullptr);
            u64 segment_id = NewCatalog::GetNextSegmentID(table_entry);
            EXPECT_EQ(segment_id, 0u);
            auto segment_entry = SegmentEntry::NewSegmentEntry(table_entry, segment_id, txn4, false);
            EXPECT_EQ(segment_entry->segment_id(), 0u);
            auto block_entry = BlockEntry::NewBlockEntry(segment_entry.get(), 0, 0, column_count, txn4);
            // auto last_block_entry = segment_entry->GetLastEntry();

            Vector<SharedPtr<ColumnVector>> columns_vector;
            {
                SharedPtr<ColumnVector> column_vector = ColumnVector::Make(MakeShared<DataType>(LogicalType::kTinyInt));
                column_vector->Initialize();
                Value v = Value::MakeTinyInt(static_cast<TinyIntT>(1));
                column_vector->AppendValue(v);
                columns_vector.push_back(column_vector);
            }
            {
                SharedPtr<ColumnVector> column = ColumnVector::Make(MakeShared<DataType>(LogicalType::kBigInt));
                column->Initialize();
                Value v = Value::MakeBigInt(static_cast<BigIntT>(22));
                column->AppendValue(v);
                columns_vector.push_back(column);
            }
            {
                SharedPtr<ColumnVector> column = ColumnVector::Make(MakeShared<DataType>(LogicalType::kDouble));
                column->Initialize();
                Value v = Value::MakeDouble(static_cast<DoubleT>(f64(3)) + 0.33f);
                column->AppendValue(v);
                columns_vector.push_back(column);
            }

            {
                auto block_column_entry0 = block_entry->GetColumnBlockEntry(0);
                auto column_type1 = block_column_entry0->column_type().get();
                EXPECT_EQ(column_type1->type(), LogicalType::kTinyInt);
                SizeT data_type_size = columns_vector[0]->data_type_size_;
                EXPECT_EQ(data_type_size, 1);
                block_column_entry0->Append(columns_vector[0].get(), 0, 1, buffer_manager);
            }
            {
                auto block_column_entry1 = block_entry->GetColumnBlockEntry(1);
                auto column_type2 = block_column_entry1->column_type().get();
                EXPECT_EQ(column_type2->type(), LogicalType::kBigInt);
                SizeT data_type_size = columns_vector[1]->data_type_size_;
                EXPECT_EQ(data_type_size, 8);
                block_column_entry1->Append(columns_vector[1].get(), 0, 1, buffer_manager);
            }
            {
                auto block_column_entry2 = block_entry->GetColumnBlockEntry(2);
                auto column_type3 = block_column_entry2->column_type().get();
                EXPECT_EQ(column_type3->type(), LogicalType::kDouble);
                SizeT data_type_size = columns_vector[2]->data_type_size_;
                EXPECT_EQ(data_type_size, 8);
                block_column_entry2->Append(columns_vector[2].get(), 0, 1, buffer_manager);
            }

            block_entry->IncreaseRowCount(1);
            segment_entry->AppendBlockEntry(std::move(block_entry));

            auto txn_store = txn4->GetTxnTableStore(table_entry);
            PhysicalImport::SaveSegmentData(txn_store, segment_entry);
            txn_mgr->CommitTxn(txn4);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
    // Restart the db instance
    system("tree  /tmp/infinity");
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BufferManager *buffer_manager = storage->buffer_manager();

        {
            auto txn = txn_mgr->CreateTxn();
            txn->Begin();
            TxnTimeStamp begin_ts = txn->BeginTS();

            Vector<ColumnID> column_ids{0, 1, 2};
            auto [table_entry, status] = txn->GetTableEntry("default", "tbl1");
            EXPECT_NE(table_entry, nullptr);
            auto *segment_entry = table_entry->GetSegmentByID(0, begin_ts);
            EXPECT_NE(segment_entry, nullptr);
            EXPECT_EQ(segment_entry->segment_id(), 0);
            auto *block_entry = segment_entry->GetBlockEntryByID(0);
            EXPECT_EQ(block_entry->block_id(), 0);
            EXPECT_EQ(block_entry->row_count(), 1);

            BlockColumnEntry *column0 = block_entry->GetColumnBlockEntry(0);
            BlockColumnEntry *column1 = block_entry->GetColumnBlockEntry(1);
            BlockColumnEntry *column2 = block_entry->GetColumnBlockEntry(2);

            ColumnVector col0 = column0->GetColumnVector(buffer_manager);
            Value v0 = col0.GetValue(0);
            EXPECT_EQ(v0.GetValue<TinyIntT>(), 1);

            ColumnVector col1 = column1->GetColumnVector(buffer_manager);
            Value v1 = col1.GetValue(0);
            EXPECT_EQ(v1.GetValue<BigIntT>(), (i64)(22));

            ColumnVector col2 = column2->GetColumnVector(buffer_manager);
            Value v2 = col2.GetValue(0);
            DataType *col2_type = column2->column_type().get();
            EXPECT_EQ(col2_type->type(), LogicalType::kDouble);
            EXPECT_EQ(v2.GetValue<DoubleT>(), (f64)(3) + 0.33f);

            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayCompact) {
    int test_segment_n = 2;
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        BufferManager *buffer_manager = storage->buffer_manager();
        TxnManager *txn_mgr = storage->txn_manager();

        Vector<SharedPtr<ColumnDef>> columns;
        {
            i64 column_id = 0;
            {
                HashSet<ConstraintType> constraints;
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id++, MakeShared<DataType>(DataType(LogicalType::kTinyInt)), "tiny_int_col", constraints);
                columns.emplace_back(column_def_ptr);
            }
        }
        int column_count = 1;
        { // create table
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("tbl1"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();

            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kIgnore);
            EXPECT_TRUE(status.ok());

            txn_mgr->CommitTxn(txn);
        }

        for (int i = 0; i < test_segment_n; ++i) { // add 2 segments
            auto txn2 = txn_mgr->CreateTxn();
            txn2->Begin();

            auto [table_entry, status] = txn2->GetTableEntry("default", "tbl1");
            EXPECT_NE(table_entry, nullptr);

            SegmentID segment_id = NewCatalog::GetNextSegmentID(table_entry);
            auto segment_entry = SegmentEntry::NewSegmentEntry(table_entry, segment_id, txn2, false);
            EXPECT_EQ(segment_entry->segment_id(), i);

            auto block_entry = BlockEntry::NewBlockEntry(segment_entry.get(), 0, 0, column_count, txn2);

            Vector<SharedPtr<ColumnVector>> column_vectors;
            {
                SharedPtr<ColumnVector> column_vector = ColumnVector::Make(MakeShared<DataType>(LogicalType::kTinyInt));
                column_vector->Initialize();
                Value v = Value::MakeTinyInt(static_cast<TinyIntT>(1));
                column_vector->AppendValue(v);
                column_vectors.push_back(column_vector);
            }

            {
                auto *block_column_entry0 = block_entry->GetColumnBlockEntry(0);
                auto column_type0 = block_column_entry0->column_type().get();
                EXPECT_EQ(column_type0->type(), LogicalType::kTinyInt);
                SizeT data_type_size = column_vectors[0]->data_type_size_;
                EXPECT_EQ(data_type_size, 1);
                block_column_entry0->Append(column_vectors[0].get(), 0, 1, buffer_manager);
                block_entry->IncreaseRowCount(1);
            }
            segment_entry->AppendBlockEntry(std::move(block_entry));

            auto txn_store = txn2->GetTxnTableStore(table_entry);
            PhysicalImport::SaveSegmentData(txn_store, segment_entry);
            txn_mgr->CommitTxn(txn2);
        }

        { // add compact
            auto txn4 = txn_mgr->CreateTxn();
            txn4->Begin();

            auto [table_entry, status] = txn4->GetTableEntry("default", "tbl1");
            EXPECT_NE(table_entry, nullptr);

            {
                auto table_ref = BaseTableRef::FakeTableRef(table_entry, txn4->BeginTS());
                auto compact_task = CompactSegmentsTask::MakeTaskWithWholeTable(table_ref, txn4);
                compact_task->Execute();
            }
            txn_mgr->CommitTxn(txn4);
        }
        infinity::InfinityContext::instance().UnInit();
        infinity::GlobalResourceUsage::UnInit();
    }
    // Restart db instance
    system("tree  /tmp/infinity");
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        {
            auto txn = txn_mgr->CreateTxn();
            txn->Begin();
            TxnTimeStamp begin_ts = txn->BeginTS();

            auto [table_entry, status] = txn->GetTableEntry("default", "tbl1");
            EXPECT_NE(table_entry, nullptr);

            for (int i = 0; i < test_segment_n; ++i) {
                auto *segment = table_entry->GetSegmentByID(i, begin_ts);
                EXPECT_NE(segment, nullptr);
                EXPECT_NE(segment->deprecate_ts(), UNCOMMIT_TS);
            }
            auto *compact_segment = table_entry->GetSegmentByID(test_segment_n, begin_ts);
            EXPECT_NE(compact_segment, nullptr);
            EXPECT_EQ(compact_segment->deprecate_ts(), UNCOMMIT_TS);
            EXPECT_EQ(compact_segment->actual_row_count(), test_segment_n);
        }
        infinity::InfinityContext::instance().UnInit();
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayCreateIndexIvfFlat) {
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        // CREATE TABLE test_annivfflat (col1 embedding(float,128));
        {
            Vector<SharedPtr<ColumnDef>> columns;
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kNotNull);
                i64 column_id = 0;
                auto embeddingInfo = MakeShared<EmbeddingInfo>(EmbeddingDataType::kElemFloat, 128);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id, MakeShared<DataType>(LogicalType::kEmbedding, embeddingInfo), "col1", constraints);
                columns.emplace_back(column_def_ptr);
            }
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("test_annivfflat"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kError);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        // CreateIndex
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();

            Vector<String> columns1{"col1"};
            Vector<InitParameter *> parameters1;
            parameters1.emplace_back(new InitParameter("centroids_count", "100"));
            parameters1.emplace_back(new InitParameter("metric", "l2"));

            auto index_base_ivf = IndexIVFFlat::Make("name1", columns1, parameters1);
            for (auto *init_parameter : parameters1) {
                delete init_parameter;
            }

            const String &db_name = "default";
            const String &table_name = "test_annivfflat";
            const SharedPtr<IndexDef> index_def = MakeShared<IndexDef>(MakeShared<String>("idx1"));
            index_def->index_array_.emplace_back(index_base_ivf);
            ConflictType conflict_type = ConflictType::kError;
            bool prepare = false;
            auto [table_entry, table_status] = txn->GetTableByName(db_name, table_name);
            EXPECT_EQ(table_status.ok(), true);
            {
                auto table_ref = BaseTableRef::FakeTableRef(table_entry, txn->BeginTS());
                auto result = txn->CreateIndexDef(table_entry, index_def, conflict_type);
                auto *table_index_entry = std::get<0>(result);
                auto status = std::get<1>(result);
                EXPECT_EQ(status.ok(), true);
                txn->CreateIndexPrepare(table_index_entry, table_ref.get(), prepare);
            }
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
    ////////////////////////////////
    /// Restart the db instance...
    ////////////////////////////////
    system("tree  /tmp/infinity");
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        {
            auto txn = txn_mgr->CreateTxn();
            txn->Begin();
            Vector<ColumnID> column_ids{0};
            auto [table_entry, status] = txn->GetTableEntry("default", "test_annivfflat");
            EXPECT_NE(table_entry, nullptr);
            auto table_index_meta = table_entry->index_meta_map()["idx1"].get();
            EXPECT_NE(table_index_meta, nullptr);
            EXPECT_EQ(table_index_meta->index_name(), "idx1");
            EXPECT_EQ(table_index_meta->entry_list().size(), 2);
            auto table_index_entry_front = static_cast<TableIndexEntry *>(table_index_meta->entry_list().front().get());
            EXPECT_EQ(*table_index_entry_front->index_def()->index_name_, "idx1");
            auto entry_back = table_index_meta->entry_list().back().get();
            EXPECT_EQ(entry_back->entry_type_, EntryType::kDummy);
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}

TEST_F(WalReplayTest, WalReplayCreateIndexHnsw) {
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();
        BufferManager *buffer_manager = storage->buffer_manager();

        // CREATE TABLE test_hnsw (col1 embedding(float,128));
        {
            Vector<SharedPtr<ColumnDef>> columns;
            {
                HashSet<ConstraintType> constraints;
                constraints.insert(ConstraintType::kNotNull);
                i64 column_id = 0;
                auto embeddingInfo = MakeShared<EmbeddingInfo>(EmbeddingDataType::kElemFloat, 128);
                auto column_def_ptr =
                    MakeShared<ColumnDef>(column_id, MakeShared<DataType>(LogicalType::kEmbedding, embeddingInfo), "col1", constraints);
                columns.emplace_back(column_def_ptr);
            }
            auto tbl1_def = MakeUnique<TableDef>(MakeShared<String>("default"), MakeShared<String>("test_hnsw"), columns);
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();
            Status status = txn->CreateTable("default", std::move(tbl1_def), ConflictType::kError);
            EXPECT_TRUE(status.ok());
            txn_mgr->CommitTxn(txn);
        }
        // CreateIndex
        {
            auto *txn = txn_mgr->CreateTxn();
            txn->Begin();

            Vector<String> columns1{"col1"};
            Vector<InitParameter *> parameters1;
            parameters1.emplace_back(new InitParameter("metric", "l2"));
            parameters1.emplace_back(new InitParameter("encode", "plain"));
            parameters1.emplace_back(new InitParameter("M", "16"));
            parameters1.emplace_back(new InitParameter("ef_construction", "200"));
            parameters1.emplace_back(new InitParameter("ef", "200"));

            auto index_base_hnsw = IndexHnsw::Make("hnsw_index", columns1, parameters1);
            for (auto *init_parameter : parameters1) {
                delete init_parameter;
            }

            const String &db_name = "default";
            const String &table_name = "test_hnsw";
            const SharedPtr<IndexDef> index_def = MakeShared<IndexDef>(MakeShared<String>("hnsw_index"));
            index_def->index_array_.emplace_back(index_base_hnsw);
            ConflictType conflict_type = ConflictType::kError;
            bool prepare = false;
            auto [table_entry, table_status] = txn->GetTableByName(db_name, table_name);
            EXPECT_EQ(table_status.ok(), true);
            {
                auto table_ref = BaseTableRef::FakeTableRef(table_entry, txn->BeginTS());
                auto result = txn->CreateIndexDef(table_entry, index_def, conflict_type);
                auto *table_index_entry = std::get<0>(result);
                auto status = std::get<1>(result);
                EXPECT_EQ(status.ok(), true);
                txn->CreateIndexPrepare(table_index_entry, table_ref.get(), prepare);
            }
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
    ////////////////////////////////
    /// Restart the db instance...
    ////////////////////////////////
    system("tree  /tmp/infinity");
    {
        infinity::GlobalResourceUsage::Init();
        std::shared_ptr<std::string> config_path = nullptr;
        infinity::InfinityContext::instance().Init(config_path);

        Storage *storage = infinity::InfinityContext::instance().storage();
        TxnManager *txn_mgr = storage->txn_manager();

        {
            auto txn = txn_mgr->CreateTxn();
            txn->Begin();
            Vector<ColumnID> column_ids{0};
            auto [table_entry, status] = txn->GetTableEntry("default", "test_hnsw");
            EXPECT_NE(table_entry, nullptr);
            auto table_index_meta = table_entry->index_meta_map()["hnsw_index"].get();
            EXPECT_NE(table_index_meta, nullptr);
            EXPECT_EQ(table_index_meta->index_name(), "hnsw_index");
            EXPECT_EQ(table_index_meta->entry_list().size(), 2);
            auto table_index_entry_front = static_cast<TableIndexEntry *>(table_index_meta->entry_list().front().get());
            EXPECT_EQ(*table_index_entry_front->index_def()->index_name_, "hnsw_index");
            auto entry_back = table_index_meta->entry_list().back().get();
            EXPECT_EQ(entry_back->entry_type_, EntryType::kDummy);
            txn_mgr->CommitTxn(txn);
        }

        infinity::InfinityContext::instance().UnInit();
        EXPECT_EQ(infinity::GlobalResourceUsage::GetObjectCount(), 0);
        EXPECT_EQ(infinity::GlobalResourceUsage::GetRawMemoryCount(), 0);
        infinity::GlobalResourceUsage::UnInit();
    }
}
