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

#include "mongosql-auth.h"
#include "mongosql-auth-conversation.h"
#include "mongosql-auth-sasl.h"
#include "mongoc/mongoc-b64.h"

void
_mongosql_auth_conversation_init(mongosql_auth_conversation_t *conv,
                                 const char *username,
                                 const char *password,
                                 const char *mechanism,
                                 const char *host) {
    char *ptr, *target_spn, *err = NULL;
    char *service_name = NULL;
    uint8_t ret;

    /* initialize fields with provided parameters  */
    conv->username = strdup(username);
    conv->password = strdup(password);
    conv->mechanism_name = strdup(mechanism);


    /* remove and store parameters from username */
    ptr = strrchr(conv->username, '?');
    if(ptr != NULL) {
        *ptr = '\0';

        // point 'ptr' to the beginning of the URL parameter list.
        ptr++;
        service_name = _mongosql_auth_conversation_find_param(ptr, "serviceName");
    }

    /* fold mechanism case */
    for (int i = 0; conv->mechanism_name[i]; i++) {
        conv->mechanism_name[i] = toupper(conv->mechanism_name[i]);
    }

    /* set defaults for other fields */
    conv->status = CR_OK;
    conv->done = 0;
    conv->buf = NULL;
    conv->buf_len = 0;
    conv->error_msg = NULL;

    #if defined(MONGOC_ENABLE_CRYPTO_CNG)
        mongoc_crypto_cng_init ();
    #endif

    if (strcmp(conv->mechanism_name, "SCRAM-SHA-1") == 0) {
        /* initialize the scram struct */
        _mongoc_scram_init(&conv->mechanism.scram, MONGOC_CRYPTO_ALGORITHM_SHA_1);
        _mongoc_scram_set_user(&conv->mechanism.scram, conv->username);
        _mongoc_scram_set_pass(&conv->mechanism.scram, conv->password);
    } else if (strcmp(conv->mechanism_name, "SCRAM-SHA-256") == 0) {
        /* initialize the scram struct */
        _mongoc_scram_init(&conv->mechanism.scram, MONGOC_CRYPTO_ALGORITHM_SHA_256);
        _mongoc_scram_set_user(&conv->mechanism.scram, conv->username);
        _mongoc_scram_set_pass(&conv->mechanism.scram, conv->password);
#ifdef MONGOSQL_AUTH_ENABLE_SASL
    } else if (strcmp(conv->mechanism_name, "GSSAPI") == 0) {
        // Default the service name if none provided.
        if (service_name == NULL) {
            #ifdef MONGOSQL_AUTH_ENABLE_SASL_GSSAPI
                target_spn = bson_strdup_printf("%s@%s", MONGOSQL_DEFAULT_SERVICE_NAME, host);
            #elif MONGOSQL_AUTH_ENABLE_SASL_SSPI
                target_spn = bson_strdup_printf("%s/%s", MONGOSQL_DEFAULT_SERVICE_NAME, host);
            #endif
        } else {
            #ifdef MONGOSQL_AUTH_ENABLE_SASL_GSSAPI
                target_spn = bson_strdup_printf("%s@%s", service_name, host);
            #elif MONGOSQL_AUTH_ENABLE_SASL_SSPI
                target_spn = bson_strdup_printf("%s/%s", service_name, host);
            #endif
            free (service_name);
        }
        mongosql_auth_log("    Target spn is: %s", target_spn);
        // Initialize the SASL struct.
        // 'err' is set to an allocated string if init does not succeed.
        ret = _mongosql_auth_sasl_init(&conv->mechanism.sasl,
                                       conv->username,
                                       conv->password,
                                       target_spn,
                                       &err);
        if (ret != SASL_OK) {
            // An error should always be set when returned, but indicate a problem if it isn't.
            if (err == NULL) {
                err = bson_strdup_printf("%s", "Failed to initialize GSSAPI mechanism. Unknown error.");
            }
            _mongosql_auth_conversation_set_error(conv, err);
            free(err);
        }
        free(target_spn);
#endif
    }
}

void
_mongosql_auth_conversation_destroy(mongosql_auth_conversation_t *conv) {

    /* destroy the scram struct */
    if (strcmp(conv->mechanism_name, "SCRAM-SHA-1") == 0 ||
        strcmp(conv->mechanism_name, "SCRAM-SHA-256") == 0) {
#ifdef MONGOSQL_AUTH_ENABLE_SASL
    } else if (strcmp(conv->mechanism_name, "GSSAPI") == 0) {
        _mongosql_auth_sasl_destroy(&conv->mechanism.sasl);
#endif
    }
    #if defined(MONGOC_ENABLE_CRYPTO_CNG)
        mongoc_crypto_cng_cleanup ();
    #endif
    /* zero the password's memory */
    memset(conv->password, 0, strlen(conv->password));

    /* free strduped strings and managed buffer */
    if (conv->buf) {
        free (conv->buf);
    }
    free(conv->username);
    free(conv->password);
    free(conv->mechanism_name);
    free(conv->error_msg);
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_step(mongosql_auth_conversation_t *conv) {
    char *err;

    if (_mongosql_auth_conversation_is_done(conv)) {
        mongosql_auth_log("%s", "Not stepping conversation: already done");
        return;
    } else if (_mongosql_auth_conversation_has_error(conv)) {
        mongosql_auth_log("%s", "Not stepping conversation: error already encountered");
        return;
    }

    if (strcmp(conv->mechanism_name, "SCRAM-SHA-1") == 0 ||
        strcmp(conv->mechanism_name, "SCRAM-SHA-256") == 0) {
        _mongosql_auth_conversation_scram_step(conv);
    } else if (strcmp(conv->mechanism_name, "PLAIN") == 0) {
        _mongosql_auth_conversation_plain_step(conv);
#ifdef MONGOSQL_AUTH_ENABLE_SASL
    } else if (strcmp(conv->mechanism_name, "GSSAPI") == 0) {
        _mongosql_auth_conversation_sasl_step(conv);
#endif
    } else {
        err = bson_strdup_printf("unsupported mechanism '%s'", conv->mechanism_name);
        mongosql_auth_log("%s", "Setting conversation error");
        _mongosql_auth_conversation_set_error(conv, err);
        free(err);
    }
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_scram_step(mongosql_auth_conversation_t *conv) {
    bson_error_t error;
    my_bool success;
    uint8_t *outbuf;
    size_t outbuf_len = 0;
    mongoc_scram_t* scram = &conv->mechanism.scram;

    mongosql_auth_log("Stepping mongosql_auth for '%s' mechanism", conv->mechanism_name);

    mongosql_auth_log("    Server challenge (%zu):", scram->step);
    mongosql_auth_log("        buf_len: %zu", conv->buf_len);
    mongosql_auth_log("        buf: %.*s", (int)conv->buf_len, conv->buf);

    outbuf = malloc(MONGOSQL_SCRAM_MAX_BUF_SIZE);
    success = _mongoc_scram_step (
        scram,
        conv->buf,
        (uint32_t) conv->buf_len,
        outbuf,
        (uint32_t)(MONGOSQL_SCRAM_MAX_BUF_SIZE * sizeof(uint8_t)),
        (uint32_t*) &outbuf_len,
        &error
    );

    // Free the input buffer.
    if (conv->buf) {
        free(conv->buf);
    }

    // The caller is responsible for managing the output buffer.
    conv->buf = outbuf;
    conv->buf_len = outbuf_len;

    if (!success) {
        _mongosql_auth_conversation_set_error(conv, "failed while executing scram step");
        return;
    }

    if (scram->step == 3) {
        conv->done = 1;
    }

    mongosql_auth_log("    Client response (%zu):", scram->step);
    mongosql_auth_log("        done: %d", conv->done);
    mongosql_auth_log("        buf_len: %zu", conv->buf_len);
    mongosql_auth_log("        buf: %.*s", (int)conv->buf_len, conv->buf);
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_plain_step(mongosql_auth_conversation_t *conv) {
    char *str;
    size_t len;

    str = bson_strdup_printf ("%c%s%c%s", '\0', conv->username, '\0', conv->password);
    len = strlen (conv->username) + strlen (conv->password) + 2;

    // Free the input and set the output to the formatted string
    if (conv->buf) {
        free(conv->buf);
    }
    conv->buf = (uint8_t*) str;
    conv->buf_len = len;
    conv->done = 1;
}

/* takes the input in buf as server input and creates the server output */
void
_mongosql_auth_conversation_sasl_step(mongosql_auth_conversation_t *conv) {
    char *error;
    uint8_t *out_buf = NULL;
    size_t out_buf_len = 0;
    uint8_t success;

    mongosql_auth_log("%s", "    Stepping mongosql_auth for GSSAPI mechanism");

    mongosql_auth_log("%s", "    Server challenge:");
    mongosql_auth_log("        buf_len: %zu", conv->buf_len);

    // On a successful return, out_buf will point to a allocated buffer that we must manage.
    // If an error occurs, 'error' will point to an string we must manage.
    success = _mongosql_auth_sasl_step (
        &conv->mechanism.sasl,
        conv->buf,
        conv->buf_len,
        &out_buf,
        &out_buf_len,
        &error
    );

    if (success != SASL_OK) {
        if (error != NULL) {
            _mongosql_auth_conversation_set_error(conv, error);
            free (error);
        } else {
            _mongosql_auth_conversation_set_error(conv, "failed while executing GSSAPI step");
        }
        return;
    }

    if (conv->mechanism.sasl.state == SASL_DONE) {
        conv->done = 1;
    }

    mongosql_auth_log("%s", "    Client response:");
    mongosql_auth_log("        done: %d", conv->done);
    mongosql_auth_log("        buf_len: %zu", out_buf_len);

    // Replace the input buffer with the output buffer from the sasl step.
    conv->buf_len = out_buf_len;
    if (conv->buf) {
        free (conv->buf);
    }
    conv->buf = out_buf;
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

char*
_mongosql_auth_conversation_find_param(char* param_list, const char* name) {
    uint32_t idx, found_len, start, result_len;
    char* result;
    char* found = strstr(param_list, name);
    if (found == NULL) {
        return NULL;
    }

    idx = strlen(name);
    // This is the length of the found string until the end of the param_list input.
    found_len = strlen(found);
    if (idx < found_len && found[idx] == '=') {
        idx++;
        start = idx;
        // Search until the next delimeter or the end of the string is found.
        while (idx < found_len && found[idx] != '\0' && found[idx] != '&') {
            ++idx;
        }

        result_len = idx - start;
        result = malloc(result_len + 1);
        memcpy(result, &found[start], result_len);
        result[result_len] = '\0';

        return result;
    }
    // The URL was malformed.
    return NULL;
}
