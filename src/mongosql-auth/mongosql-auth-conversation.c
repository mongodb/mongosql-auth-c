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

#include "mongoc/mongoc-b64.h"
#include "mongosql-auth-conversation.h"

void
_mongosql_auth_conversation_init(mongosql_auth_conversation_t *conv, const char *username, const char *password, const char *mechanism) {
    char *ptr;

    /* initialize fields with provided parameters  */
    conv->username = strdup(username);
    conv->password = strdup(password);
    conv->mechanism = strdup(mechanism);

    /* remove parameters from username */
    ptr = strrchr(conv->username, '?');
    if(ptr != NULL) {
        *ptr = '\0';
    }

    /* fold mechanism case */
    for (int i = 0; conv->mechanism[i]; i++) {
        conv->mechanism[i] = toupper(conv->mechanism[i]);
    }

    /* initialize the scram struct */
    _mongoc_scram_init(&conv->scram);
    _mongoc_scram_set_user(&conv->scram, conv->username);
    _mongoc_scram_set_pass(&conv->scram, conv->password);

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

    /* zero the password's memory */
    memset(conv->password, 0, strlen(conv->password));

    /* free strduped strings */
    free(conv->username);
    free(conv->password);
    free(conv->mechanism);
    free(conv->error_msg);
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_step(mongosql_auth_conversation_t *conv) {
    char *err;

    if (_mongosql_auth_conversation_is_done(conv)) {
        MONGOSQL_AUTH_LOG("%s", "Not stepping conversation: already done");
        return;
    } else if (_mongosql_auth_conversation_has_error(conv)) {
        MONGOSQL_AUTH_LOG("%s", "Not stepping conversation: error already encountered");
        return;
    }

    if(strcmp(conv->mechanism, "SCRAM-SHA-1") == 0) {
        _mongosql_auth_conversation_scram_step(conv);
    } else if(strcmp(conv->mechanism, "PLAIN") == 0) {
        _mongosql_auth_conversation_plain_step(conv);
    } else {
        err = bson_strdup_printf("unsupported mechanism '%s'", conv->mechanism);
        MONGOSQL_AUTH_LOG("%s", "Setting conversation error");
        _mongosql_auth_conversation_set_error(conv, err);
        free(err);
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

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_plain_step(mongosql_auth_conversation_t *conv) {
    unsigned char *str;
    size_t len;

    str = bson_strdup_printf ("%c%s%c%s", '\0', conv->username, '\0', conv->password);
    len = strlen (conv->username) + strlen (conv->password) + 2;

    memcpy(conv->buf, str, len);
    conv->buf_len = len;

    free(str);
    conv->done = 1;
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
