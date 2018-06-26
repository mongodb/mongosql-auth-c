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

#ifndef MONGOSQL_AUTH_SASL_H
#define MONGOSQL_AUTH_SASL_H

#define SASL_OK 0
#define SASL_CONTINUE 1
#define SASL_ERR 2

#include <stdint.h>
#include <stddef.h>

#ifdef MONGOSQL_AUTH_ENABLE_SASL_GSSAPI
#include "mongosql-auth-gssapi.h"
#elif MONGOSQL_AUTH_ENABLE_SASL_SSPI
#include "mongosql-auth-sspi.h"
#endif

typedef enum {
    SASL_START,
    SASL_CONTEXT_COMPLETE,
    SASL_DONE
} mongosql_auth_sasl_state;

// Transparently provide a structure to wrap the underling GSSAPI implementation.
typedef struct {
#ifdef MONGOSQL_AUTH_ENABLE_SASL_GSSAPI
       mongosql_auth_gssapi_client client;
#elif MONGOSQL_AUTH_ENABLE_SASL_SSPI
       mongosql_auth_sspi_client client;
#endif
       mongosql_auth_sasl_state state;
} mongosql_auth_sasl_client;


uint8_t _mongosql_auth_sasl_init(mongosql_auth_sasl_client* sasl,
                                 char* username,
                                 char* password,
                                 char* target_spn,
                                 char** error);

uint8_t _mongosql_auth_sasl_step(mongosql_auth_sasl_client* sasl,
                                  uint8_t *inbuf,
                                  size_t inbuflen,
                                  uint8_t **outbuf,
                                  size_t *outbuflen,
                                  char **error);

uint8_t _mongosql_auth_sasl_destroy(mongosql_auth_sasl_client* sasl);

#endif /* MONGOSQL_AUTH_SASL_H */
