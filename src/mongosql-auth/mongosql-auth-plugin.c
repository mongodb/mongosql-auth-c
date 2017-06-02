/*
 * Copyright 2017 MongoDB, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <mysql/plugin_auth.h>
#include <mysql/client_plugin.h>
#include <mysql/service_my_plugin_log.h>

#include "mongosql-auth-plugin.h"
#include "mongoc/mongoc-b64.h"
#include "mongoc/mongoc-scram.h"

#define MAX_MECHANISM_LENGTH 1024

/**
  Authenticate the client using the MongoDB MySQL Authentication Plugin Protocol.

  @param vio Provides plugin access to communication channel
  @param mysql Client connection handler

  @return Error status
    @retval CR_ERROR An error occurred.
    @retval CR_OK Authentication succeeded.
*/
static int mongosql_auth(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql)
{
    return CR_OK;
}

int auth_scram(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations) {
    return CR_OK;
}

int auth_plain(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql, uint32_t num_conversations) {
    return CR_OK;
}

mysql_declare_client_plugin(AUTHENTICATION)
    "mongosql_auth",
    "MongoDB",
    "MongoDB MySQL Authentication Plugin",
    {0,1,0},
    "Apache License, Version 2.0",
    NULL,
    NULL,
    NULL,
    NULL,
    mongosql_auth
mysql_end_client_plugin;
