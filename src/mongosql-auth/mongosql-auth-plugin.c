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
#include <mysql.h>
#include "mongosql-auth-config.h"
#include "mongosql-auth.h"
#include "mongosql-auth-plugin.h"

/**
  Authenticate the client using the MongoDB MySQL Authentication Plugin Protocol.

  @param vio Provides plugin access to communication channel
  @param mysql Client connection handler

  @return Error status
    @retval CR_ERROR An error occurred.
    @retval CR_OK Authentication succeeded.
*/
int mongosql_auth(MYSQL_PLUGIN_VIO *vio, MYSQL *mysql)
{
    mongosql_auth_t plugin;
    int status;

    _mongosql_auth_init(&plugin, vio);
    _mongosql_auth_start(&plugin, mysql->user, mysql->passwd, mysql->host);

    while (!_mongosql_auth_is_done(&plugin)) {
        _mongosql_auth_step(&plugin);
        _mongosql_auth_write_payload(&plugin);
        _mongosql_auth_read_payload(&plugin);
    }

    status = plugin.status;
    if (status == CR_OK) {
        MONGOSQL_AUTH_LOG("%s", "Authentication finished successfully");
    } else {
        MONGOSQL_AUTH_LOG("%s", "Authentication was unsuccessful");
        MONGOSQL_AUTH_LOG("Error message: '%s'", plugin.error_msg);
    }

    _mongosql_auth_destroy(&plugin);

    return status;
}

mysql_declare_client_plugin(AUTHENTICATION)
    "mongosql_auth",
    "MongoDB",
    "MongoDB MySQL Authentication Plugin",
    {1,1,0},
    "Apache License, Version 2.0",
    NULL,
    NULL,
    NULL,
    NULL,
    mongosql_auth
mysql_end_client_plugin;
