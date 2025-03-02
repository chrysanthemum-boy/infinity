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

export module db_meta;

import stl;

import buffer_manager;
import third_party;
import status;
import extra_ddl_info;
import db_entry;
import base_entry;
import txn_manager;

namespace infinity {

struct NewCatalog;

export struct DBMeta {

    friend struct NewCatalog;

public:
    explicit DBMeta(const SharedPtr<String> &data_dir, SharedPtr<String> db_name) : db_name_(std::move(db_name)), data_dir_(data_dir) {}

    static UniquePtr<DBMeta> NewDBMeta(const SharedPtr<String> &data_dir, const SharedPtr<String> &db_name);

    SharedPtr<String> ToString();

    nlohmann::json Serialize(TxnTimeStamp max_commit_ts, bool is_full_checkpoint);

    static UniquePtr<DBMeta> Deserialize(const nlohmann::json &db_meta_json, BufferManager *buffer_mgr);

    void MergeFrom(DBMeta &other);

    SharedPtr<String> db_name() const { return db_name_; }

    SharedPtr<String> data_dir() const { return data_dir_; }

private:
    Tuple<DBEntry *, Status>
    CreateNewEntry(TransactionID txn_id, TxnTimeStamp begin_ts, TxnManager *txn_mgr, ConflictType conflict_type = ConflictType::kError);

    Tuple<DBEntry *, Status> DropNewEntry(TransactionID txn_id, TxnTimeStamp begin_ts, TxnManager *txn_mgr);

    void DeleteNewEntry(TransactionID txn_id, TxnManager *txn_mgr);

    Tuple<DBEntry *, Status> GetEntry(TransactionID txn_id, TxnTimeStamp begin_ts);

    Tuple<DBEntry *, Status> GetEntryReplay(TransactionID txn_id, TxnTimeStamp begin_ts);
    // Thread-unsafe
    List<SharedPtr<BaseEntry>> &entry_list() { return entry_list_; }

    // Used in initialization phase
    static void AddEntry(DBMeta *db_meta, UniquePtr<BaseEntry> db_entry);

private:
    SharedPtr<String> db_name_{};
    SharedPtr<String> data_dir_{};

    std::shared_mutex rw_locker_{};
    // Ordered by commit_ts from latest to oldest.
    List<SharedPtr<BaseEntry>> entry_list_{};
};

} // namespace infinity
