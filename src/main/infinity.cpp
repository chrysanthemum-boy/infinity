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

#include <iostream>

module infinity;

import stl;
import logger;
import config;
import resource_manager;
import task_scheduler;
import storage;
import local_file_system;
import third_party;
import query_options;
import query_result;
import database;
import infinity_context;
import session;
import session_manager;
import query_context;

import status;
import create_statement;
import show_statement;
import drop_statement;
import command_statement;
import flush_statement;
import create_schema_info;
import drop_schema_info;

namespace infinity {

SharedPtr<Infinity> Infinity::RemoteConnect() {
    SharedPtr<Infinity> infinity_ptr = MakeShared<Infinity>();
    SessionManager* session_mgr = InfinityContext::instance().session_manager();
    infinity_ptr->session_ = session_mgr->CreateRemoteSession();
    return infinity_ptr;
}

void Infinity::RemoteDisconnect() {
    SessionManager* session_mgr = InfinityContext::instance().session_manager();
    session_mgr->RemoveSessionByID(session_->session_id());
    session_.reset();
}

u64 Infinity::GetSessionId() {
    return session_->session_id();
}

void Infinity::LocalInit(const String &path) {
    LocalFileSystem fs;
    if (!fs.Exists(path)) {
        std::cerr << path << " doesn't exist." << std::endl;
        return;
    }

    SharedPtr<String> config_path = MakeShared<String>(path + "/infinity_conf.toml");

    InfinityContext::instance().Init(config_path);
}

void Infinity::LocalUnInit() {
    InfinityContext::instance().UnInit();
}

SharedPtr<Infinity> Infinity::LocalConnect() {
    SharedPtr<Infinity> infinity_ptr = MakeShared<Infinity>();

    SessionManager* session_mgr = InfinityContext::instance().session_manager();
    infinity_ptr->session_ = session_mgr->CreateLocalSession();
    return infinity_ptr;
}

void Infinity::LocalDisconnect() {
//    fmt::print("To disconnect the database.\n");
}

QueryResult Infinity::CreateDatabase(const String &db_name, const CreateDatabaseOptions &create_db_options) {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    UniquePtr<CreateStatement> create_statement = MakeUnique<CreateStatement>();
    SharedPtr<CreateSchemaInfo> create_schema_info = MakeShared<CreateSchemaInfo>();
    create_schema_info->schema_name_ = db_name;
    create_statement->create_info_ = create_schema_info;

    create_statement->create_info_->conflict_type_ = create_db_options.conflict_type_;
    QueryResult query_result = query_context_ptr->QueryStatement(create_statement.get());
    return query_result;
}

QueryResult Infinity::DropDatabase(const String &db_name, const DropDatabaseOptions &) {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    UniquePtr<DropStatement> drop_statement = MakeUnique<DropStatement>();
    SharedPtr<DropSchemaInfo> drop_schema_info = MakeShared<DropSchemaInfo>();
    drop_schema_info->schema_name_ = db_name;
    drop_statement->drop_info_ = drop_schema_info;
    QueryResult result = query_context_ptr->QueryStatement(drop_statement.get());
    return result;
}

QueryResult Infinity::ListDatabases() {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    UniquePtr<ShowStatement> show_statement = MakeUnique<ShowStatement>();
    show_statement->show_type_ = ShowStmtType::kDatabases;
    QueryResult result = query_context_ptr->QueryStatement(show_statement.get());
    return result;
}

UniquePtr<Database> Infinity::GetDatabase(const String &db_name) {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    UniquePtr<CommandStatement> command_statement = MakeUnique<CommandStatement>();
    command_statement->command_info_ = MakeUnique<UseCmd>(db_name.c_str());
    QueryResult result = query_context_ptr->QueryStatement(command_statement.get());
    if (result.status_.ok()) {
        return MakeUnique<Database>(db_name, session_);
    } else {
        return nullptr;
    }
}

QueryResult Infinity::Query(const String &query_text) {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    QueryResult result = query_context_ptr->Query(query_text);
    return result;
}

QueryResult Infinity::Flush() {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());
    UniquePtr<FlushStatement> flush_statement = MakeUnique<FlushStatement>();
    flush_statement->type_ = FlushType::kData;

    QueryResult result = query_context_ptr->QueryStatement(flush_statement.get());
    return result;
}

QueryResult Infinity::ShowVariable(const String& variable_name) {
    UniquePtr<QueryContext> query_context_ptr = MakeUnique<QueryContext>(session_.get());
    query_context_ptr->Init(InfinityContext::instance().config(),
                            InfinityContext::instance().task_scheduler(),
                            InfinityContext::instance().storage(),
                            InfinityContext::instance().resource_manager(),
                            InfinityContext::instance().session_manager());

    UniquePtr<ShowStatement> show_statement = MakeUnique<ShowStatement>();
    show_statement->var_name_ = variable_name;
    show_statement->show_type_ = ShowStmtType::kVar;
    QueryResult result = query_context_ptr->QueryStatement(show_statement.get());
    return result;
}

} // namespace infinity
