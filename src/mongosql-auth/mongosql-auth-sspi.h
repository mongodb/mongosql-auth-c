
#ifndef MONGOSQL_AUTH_SSPI_H
#define MONGOSQL_AUTH_SSPI_H

#define SECURITY_WIN32 1  /* Required for SSPI */

#include <windows.h>
#include <sspi.h>

#define SSPI_OK 0
#define SSPI_CONTINUE 1
#define SSPI_ERROR 2

typedef struct {
    char* spn;

    CredHandle cred;
    CtxtHandle ctx;

    int has_ctx;

    SECURITY_STATUS status;
} mongosql_auth_sspi_client;

int sspi_init();

int mongosql_auth_sspi_client_init(
    mongosql_auth_sspi_client *client,
    char* username,
    char* password,
    char* target_spn
);

int mongosql_auth_sspi_client_username(
    mongosql_auth_sspi_client *client,
    char** username
);

int mongosql_auth_sspi_client_negotiate(
    mongosql_auth_sspi_client *client,
    PVOID input,
    ULONG input_length,
    PVOID* output,
    ULONG* output_length
);

int mongosql_auth_sspi_client_wrap_msg(
    mongosql_auth_sspi_client *client,
    PVOID input,
    ULONG input_length,
    PVOID* output,
    ULONG* output_length
);

int mongosql_auth_sspi_client_destroy(
    mongosql_auth_sspi_client *client
);

#endif /* MONGOSQL_AUTH_SSPI_H */
