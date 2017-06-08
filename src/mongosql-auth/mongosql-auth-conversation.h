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

#ifndef MONGOSQL_AUTH_CONVERSATION_H
#define MONGOSQL_AUTH_CONVERSATION_H

#include <mysql/client_plugin.h>
#include <my_global.h>
#include <stdint.h>
#include "mongosql-auth-config.h"
#include "mongoc/mongoc-scram.h"

typedef struct mongosql_auth_conversation_t {
    mongoc_scram_t scram;
    char *mechanism;
    uint8_t done;
    unsigned char buf[4096];
    uint32_t buf_len;
    char* error_msg;
    int status;
} mongosql_auth_conversation_t;

void
_mongosql_auth_conversation_init(mongosql_auth_conversation_t *conv, const char *username, const char *password, const char *mechanism);

void
_mongosql_auth_conversation_destroy(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_step(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_scram_step(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_set_error(mongosql_auth_conversation_t *conv, const char *msg);

my_bool
_mongosql_auth_conversation_has_error(mongosql_auth_conversation_t *conv);

my_bool
_mongosql_auth_conversation_is_done(mongosql_auth_conversation_t *conv);

#endif /* MONGOSQL_AUTH_CONVERSATION_H */
