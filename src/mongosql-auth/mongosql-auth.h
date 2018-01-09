/*
 * Copyright 2017 MongoDB Inc.
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

#ifndef MONGOSQL_AUTH_H
#define MONGOSQL_AUTH_H

#include <mysql/client_plugin.h>
#include <my_global.h>
#include <stdint.h>
#include "mongosql-auth-config.h"
#include "mongosql-auth-conversation.h"

#define MONGOSQL_AUTH_MAX_BUF_SIZE 65536

typedef struct mongosql_auth_t {
    int status;
    char* error_msg;
    uint32_t num_conversations;
    mongosql_auth_conversation_t *conversations;
    MYSQL_PLUGIN_VIO *vio;
} mongosql_auth_t;

void
_mongosql_auth_init(mongosql_auth_t *plugin, MYSQL_PLUGIN_VIO *vio);

void
_mongosql_auth_destroy(mongosql_auth_t *plugin);

void
_mongosql_auth_start(mongosql_auth_t *plugin, const char *username, const char *password, const char *host);

void
_mongosql_auth_step(mongosql_auth_t *plugin);

void
_mongosql_auth_read_payload(mongosql_auth_t *plugin);

void
_mongosql_auth_write_payload(mongosql_auth_t *plugin);

void
_mongosql_auth_set_error(mongosql_auth_t *plugin, const char *msg);

my_bool
_mongosql_auth_has_error(mongosql_auth_t *plugin);

my_bool
_mongosql_auth_is_done(mongosql_auth_t *plugin);

#endif /* MONGOSQL_AUTH_H */
