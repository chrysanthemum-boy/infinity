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

export module txn;

import stl;

import table_detail;
import table_def;
import index_def;
import data_block;
import meta_state;
import data_access_state;
import buffer_manager;
import txn_state;
import txn_context;
import txn_store;
import database_detail;
import status;
import extra_ddl_info;
import internal_types;

namespace infinity {

struct GetParam {
    const String &db_name_{};
    const String &table_name_{};
    const Vector<ColumnID> &column_ids_{};
};

struct ScanParam {
    const String &db_name_{};
    const String &table_name_{};
    const Vector<ColumnID> &column_ids_{};
};

class TxnManager;
struct NewCatalog;
class BGTaskProcessor;
struct TableEntry;
struct DBEntry;
struct BaseEntry;
struct TableIndexEntry;
struct SegmentEntry;
struct WalEntry;
struct WalCmd;
class CatalogDeltaEntry;
class CatalogDeltaOperation;
class BaseTableRef;
enum class CompactSegmentsTaskType;

export class Txn : public EnableSharedFromThis<Txn> {
public:
    explicit Txn(TxnManager *txn_manager,
                 BufferManager *buffer_manager,
                 NewCatalog *catalog,
                 BGTaskProcessor *bg_task_processor,
                 TransactionID txn_id);

    explicit Txn(BufferManager *buffer_mgr, TxnManager *txn_mgr, NewCatalog *catalog, TransactionID txn_id);

    static UniquePtr<Txn> NewReplayTxn(BufferManager *buffer_mgr, TxnManager *txn_mgr, NewCatalog *catalog, TransactionID txn_id);

    // Txn OPs
    void Begin();

    TxnTimeStamp Commit();

    void CommitBottom() noexcept;

    void CancelCommitBottom();

    void Rollback();

    // Database OPs
    Status CreateDatabase(const String &db_name, ConflictType conflict_type);

    Status DropDatabase(const String &db_name, ConflictType conflict_type);

    Tuple<DBEntry *, Status> GetDatabase(const String &db_name);

    Vector<DatabaseDetail> ListDatabases();

    // Table and Collection OPs
    Status GetTables(const String &db_name, Vector<TableDetail> &output_table_array);

    Status CreateTable(const String &db_name, const SharedPtr<TableDef> &table_def, ConflictType conflict_type);

    Status CreateCollection(const String &db_name, const String &collection_name, ConflictType conflict_type, BaseEntry *&collection_entry);

    Status DropTableCollectionByName(const String &db_name, const String &table_name, ConflictType conflict_type);

    Tuple<TableEntry *, Status> GetTableByName(const String &db_name, const String &table_name);

    Status GetCollectionByName(const String &db_name, const String &table_name, BaseEntry *&collection_entry);

    Tuple<TableEntry *, Status> GetTableEntry(const String &db_name, const String &table_name);

    // Index OPs
    // If `prepare` is false, the index will be created in single thread. (called by `FsPhysicalCreateIndex`)
    // Else, only data is stored in index (Called by `PhysicalCreateIndexPrepare`). And the index will be created by multiple threads in next
    // operator. (called by `PhysicalCreateIndexDo`)
    Tuple<TableIndexEntry *, Status> CreateIndexDef(TableEntry *table_entry, const SharedPtr<IndexDef> &index_def, ConflictType conflict_type);

    Status CreateIndexPrepare(TableIndexEntry *table_index_entry, BaseTableRef *table_ref, bool prepare, bool check_ts = true);

    Status CreateIndexDo(BaseTableRef *table_ref, const String &index_name, HashMap<SegmentID, atomic_u64> &create_index_idxes);

    // write wal
    Status CreateIndexFinish(const String &db_name, const String &table_name, const SharedPtr<IndexDef> &indef);

    Status DropIndexByName(const String &db_name, const String &table_name, const String &index_name, ConflictType conflict_type);

    // View Ops
    // Fixme: view definition should be given
    Status CreateView(const String &db_name, const String &view_name, ConflictType conflict_type, BaseEntry *&view_entry);

    Status DropViewByName(const String &db_name, const String &view_name, ConflictType conflict_type, BaseEntry *&view_entry);

    Status GetViewByName(const String &db_name, const String &view_name, BaseEntry *&view_entry);

    Status GetViews(const String &db_name, Vector<ViewDetail> &output_view_array);

    // DML
    Status Append(const String &db_name, const String &table_name, const SharedPtr<DataBlock> &input_block);

    Status Delete(const String &db_name, const String &table_name, const Vector<RowID> &row_ids, bool check_conflict = true);

    Status Compact(const String &db_name,
                   const String &table_name,
                   Vector<Pair<SharedPtr<SegmentEntry>, Vector<SegmentEntry *>>> &&segment_data,
                   CompactSegmentsTaskType type);

    // Getter
    BufferManager *GetBufferMgr() const;

    BufferManager *buffer_manager() const { return buffer_mgr_; }

    NewCatalog *GetCatalog() { return catalog_; }

    inline TransactionID TxnID() const { return txn_id_; }

    inline TxnTimeStamp CommitTS() { return txn_context_.GetCommitTS(); }

    inline TxnTimeStamp BeginTS() { return txn_context_.GetBeginTS(); }

    inline TxnState GetTxnState() { return txn_context_.GetTxnState(); }

    void SetTxnCommitted() { txn_context_.SetTxnCommitted(); }

    // WAL and replay OPS
    // Dangerous! only used during replaying wal.
    void FakeCommit(TxnTimeStamp commit_ts);

    // Create txn store if not exists
    TxnTableStore *GetTxnTableStore(TableEntry *table_entry);

    void AddWalCmd(const SharedPtr<WalCmd> &cmd);

    void AddCatalogDeltaOperation(UniquePtr<CatalogDeltaOperation> operation);

    void Checkpoint(const TxnTimeStamp max_commit_ts, bool is_full_checkpoint);

    void FullCheckpoint(const TxnTimeStamp max_commit_ts);

    void DeltaCheckpoint(const TxnTimeStamp max_commit_ts);

    TxnManager *txn_mgr() const { return txn_mgr_; }

private:
    TxnManager *txn_mgr_{};
    // This BufferManager ptr Only for replaying wal
    BufferManager *buffer_mgr_{};
    BGTaskProcessor *bg_task_processor_{};
    NewCatalog *catalog_{};
    TransactionID txn_id_{};

    TxnContext txn_context_{};

    // Related database
    Set<String> db_names_{};

    // Txn store
    Set<DBEntry *> txn_dbs_{};
    Set<TableEntry *> txn_tables_{};
    HashMap<String, TableIndexEntry *> txn_indexes_{};

    // Only one db can be handled in one transaction.
    HashMap<String, BaseEntry *> txn_table_entries_{};
    // Key: table name Value: TxnTableStore
    HashMap<String, SharedPtr<TxnTableStore>> txn_tables_store_{};

    // Handled database
    String db_name_{};

    /// LOG
    // WalEntry
    SharedPtr<WalEntry> wal_entry_{};

    UniquePtr<CatalogDeltaEntry> local_catalog_delta_ops_entry_{};

    // WalManager notify the  commit bottom half is done
    std::mutex lock_{};
    std::condition_variable cond_var_{};
    bool done_bottom_{false};
};

} // namespace infinity
