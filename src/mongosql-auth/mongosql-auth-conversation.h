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
#include "mongoc/mongoc-scram.h"
#include "mongosql-auth-config.h"
#include "mongosql-auth-sasl.h"

#define MONGOSQL_SCRAM_MAX_BUF_SIZE 4096
#define MONGOSQL_DEFAULT_SERVICE_NAME "mongosql"


typedef struct mongosql_auth_conversation_t {
    char* mechanism_name;
    union {
        /* mechanism_name: SCRAM-SHA-1 */
        mongoc_scram_t scram;
#ifdef MONGOSQL_AUTH_ENABLE_SASL
        /* mechanism_name: GSSAPI */
        mongosql_auth_sasl_client sasl;
#endif /* MONGOSQL_AUTH_ENABLE_SASL */
    } mechanism;
    /* mechanism_name: PLAIN */
    char* username;
    char* password;
    uint8_t done;
    uint8_t *buf;
    size_t buf_len;
    char* error_msg;
    int status;
} mongosql_auth_conversation_t;

void
_mongosql_auth_conversation_init(mongosql_auth_conversation_t *conv,
                                 const char *username,
                                 const char *password,
                                 const char *mechanism,
                                 const char *host);

void
_mongosql_auth_conversation_destroy(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_step(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_scram_step(mongosql_auth_conversation_t *conv);

void
_mongosql_auth_conversation_plain_step(mongosql_auth_conversation_t *conv);

#ifdef MONGOSQL_AUTH_ENABLE_SASL
void
_mongosql_auth_conversation_sasl_step(mongosql_auth_conversation_t *conv);
#endif

void
_mongosql_auth_conversation_set_error(mongosql_auth_conversation_t *conv, const char *msg);

my_bool
_mongosql_auth_conversation_has_error(mongosql_auth_conversation_t *conv);

my_bool
_mongosql_auth_conversation_is_done(mongosql_auth_conversation_t *conv);

char*
_mongosql_auth_conversation_find_param(char* param_list, const char* name);

#endif /* MONGOSQL_AUTH_CONVERSATION_H */
