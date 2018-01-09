#ifndef MONGOSQL_AUTH_GSSAPI_H
#define MONGOSQL_AUTH_GSSAPI_H

#include <stdlib.h>
#ifdef __linux__
#include <gssapi/gssapi.h>
#include <gssapi/gssapi_krb5.h>
#endif
#ifdef __APPLE__
#include <GSS/GSS.h>
#endif

#define GSSAPI_OK 0
#define GSSAPI_CONTINUE 1
#define GSSAPI_ERROR 2

typedef struct {
    gss_name_t spn;
    gss_cred_id_t cred;
    gss_ctx_id_t ctx;

    OM_uint32 maj_stat;
    OM_uint32 min_stat;
} mongosql_auth_gssapi_client;

void mongosql_auth_gssapi_log_error(
    mongosql_auth_gssapi_client *client,
    const char *prefix,
    char **errmsg
);

int mongosql_auth_gssapi_error_desc(
    OM_uint32 maj_stat,
    OM_uint32 min_stat,
    char **desc
);

OM_uint32 mongosql_auth_gssapi_canonicalize_name(
    OM_uint32* minor_status,
    char *input_name,
    gss_OID input_name_type,
    gss_name_t *output_name
);

OM_uint32 mongosql_copy_and_release_buffer(
    OM_uint32* minor_status,
    gss_buffer_desc* buffer,
    uint8_t** output,
    size_t* output_length
);

int mongosql_auth_gssapi_client_init(
    mongosql_auth_gssapi_client *client,
    char* username,
    char* password,
    char* target_spn
);

int mongosql_auth_gssapi_client_username(
    mongosql_auth_gssapi_client *client,
    char** username
);

int mongosql_auth_gssapi_client_negotiate(
    mongosql_auth_gssapi_client *client,
    uint8_t* input,
    size_t input_length,
    uint8_t** output,
    size_t* output_length
);

int mongosql_auth_gssapi_client_wrap_msg(
    mongosql_auth_gssapi_client *client,
    uint8_t* input,
    size_t input_length,
    uint8_t** output,
    size_t* output_length 
);

int mongosql_auth_gssapi_client_destroy(
    mongosql_auth_gssapi_client *client
);

#endif // MONGOSQL_AUTH_GSSAPI_H
