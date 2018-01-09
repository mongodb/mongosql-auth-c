
#include "mongosql-auth-sasl.h"
#include "mongosql-auth-sspi.h"

#define _SSPI_NOT_SUPPORTED "SSPI is not implemented yet."

uint8_t _mongosql_auth_sasl_init(mongosql_auth_sasl_client* sasl,
                                 char* username,
                                 char* password,
                                 char* target_spn,
                                 char** error) {
    *error = malloc(strlen(_SSPI_NOT_SUPPORTED) + 1);
    strcpy(*error, _SSPI_NOT_SUPPORTED);
    return SASL_ERR;
}
uint8_t _mongosql_auth_sasl_step(mongosql_auth_sasl_client* sasl,
                                  uint8_t *inbuf,
                                  size_t inbuflen,
                                  uint8_t **outbuf,
                                  size_t *outbuflen,
                                  char **error) {
    *error = malloc(strlen(_SSPI_NOT_SUPPORTED) + 1);
    strcpy(*error, _SSPI_NOT_SUPPORTED);
    return SASL_ERR;
}
uint8_t _mongosql_auth_sasl_destroy(mongosql_auth_sasl_client* sasl) {
    return SASL_ERR;
}

