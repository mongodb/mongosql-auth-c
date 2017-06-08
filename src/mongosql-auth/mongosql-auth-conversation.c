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

#include "mongosql-auth-conversation.h"

void
_mongosql_auth_conversation_init(mongosql_auth_conversation_t *conv, const char *username, const char *password, const char *mechanism) {

    /* initialize the scram struct */
    _mongoc_scram_init(&conv->scram);
    _mongoc_scram_set_user(&conv->scram, username);
    _mongoc_scram_set_pass(&conv->scram, password);

    /* initialize fields with provided parameters  */
    conv->mechanism = strdup(mechanism);

    /* set defaults for other fields */
    conv->status = CR_OK;
    conv->done = 0;
    conv->buf_len = 0;
    conv->error_msg = NULL;
}

void
_mongosql_auth_conversation_destroy(mongosql_auth_conversation_t *conv) {

    /* destroy the scram struct */
    _mongoc_scram_destroy(&conv->scram);

    /* free strduped strings */
    free(conv->mechanism);
    free(conv->error_msg);
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_step(mongosql_auth_conversation_t *conv) {
    if (_mongosql_auth_conversation_is_done(conv)) {
        return;
    } else if (_mongosql_auth_conversation_has_error(conv)) {
        return;
    }

    if(strcmp(conv->mechanism, "SCRAM-SHA-1") == 0) {
        _mongosql_auth_conversation_scram_step(conv);
    } else {
        _mongosql_auth_conversation_set_error(conv, "unsupported mechanism");
    }
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_scram_step(mongosql_auth_conversation_t *conv) {
    bson_error_t error;
    my_bool success;

    MONGOSQL_AUTH_LOG("%s", "    Stepping mongosql_auth for SCRAM-SHA-1 mechanism");

    MONGOSQL_AUTH_LOG("%s", "    Server challenge:");
    MONGOSQL_AUTH_LOG("        buf_len: %d", conv->buf_len);
    MONGOSQL_AUTH_LOG("        buf: %.*s", conv->buf_len, conv->buf);

    success = _mongoc_scram_step (
        &conv->scram,
        conv->buf,
        conv->buf_len,
        conv->buf,
        (uint32_t)(4096 * sizeof(uint8_t)),
        &conv->buf_len,
        &error
    );

    if (!success) {
        _mongosql_auth_conversation_set_error(conv, "failed while executing scram step");
        return;
    }

    if (conv->scram.step == 3) {
        conv->done = 1;
    }

    MONGOSQL_AUTH_LOG("%s", "    Client response:");
    MONGOSQL_AUTH_LOG("        done: %d", conv->done);
    MONGOSQL_AUTH_LOG("        buf_len: %d", conv->buf_len);
    MONGOSQL_AUTH_LOG("        buf: %.*s", conv->buf_len, conv->buf);
}

void
_mongosql_auth_conversation_set_error(mongosql_auth_conversation_t *conv, const char *msg) {
    if (_mongosql_auth_conversation_has_error(conv)) {
        return;
    }
    conv->error_msg = strdup(msg);
    conv->status = CR_ERROR;
}

my_bool
_mongosql_auth_conversation_has_error(mongosql_auth_conversation_t *conv) {
    if (conv->status == CR_ERROR) {
        return TRUE;
    }
    return FALSE;
}

my_bool
_mongosql_auth_conversation_is_done(mongosql_auth_conversation_t *conv) {
    if (conv->done == 1) {
        return TRUE;
    }
    return FALSE;
}
