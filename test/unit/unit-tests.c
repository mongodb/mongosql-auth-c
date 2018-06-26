
#include <my_global.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unit-tests.h"
#include "mongosql-auth.h"
#include "mongosql-auth-sasl.h"
#include "mongoc/mongoc-misc.h"
#include "mongoc/mongoc-scram.h"

int
main (int argc, char *argv[]) {

    int ret = 0;

    ret += test_mongosql_auth_conversation_scram_parameters();

    return ret;
}

int test_mongosql_auth_conversation_scram_parameters () {
    mongosql_auth_conversation_t scram_sha_1_conv;

    fprintf(stderr, "Testing mongosql_auth_conversation_t SCRAM-SHA-1 parameter initialization...");

    _mongosql_auth_conversation_init(&scram_sha_1_conv, "username?source=mydb", "password", "scram-sha-1", NULL);

    if (strcmp(scram_sha_1_conv.username, "username")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.username to be 'username', got '%s'\n", scram_sha_1_conv.username);
        return 1;
    }

    if (strcmp(scram_sha_1_conv.password, "password")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.password to be 'password', got '%s'\n", scram_sha_1_conv.password);
        return 1;
    }

    if (strcmp(scram_sha_1_conv.mechanism_name, "SCRAM-SHA-1")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.mechanism_name to be 'SCRAM-SHA-1', got '%s'\n", scram_sha_1_conv.mechanism_name);
        return 1;
    }

    if (scram_sha_1_conv.status != CR_OK) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.status to be '%d', got '%d'\n", CR_OK, scram_sha_1_conv.status);
        return 1;
    }

    mongosql_auth_conversation_t scram_sha_256_conv;

    fprintf(stderr, "Testing mongosql_auth_conversation_t SCRAM-SHA-256 parameter initialization...");

    _mongosql_auth_conversation_init(&scram_sha_256_conv, "username?source=mydb", "password", "scram-sha-256", NULL);

    if (strcmp(scram_sha_256_conv.username, "username")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.username to be 'username', got '%s'\n", scram_sha_256_conv.username);
        return 1;
    }

    if (strcmp(scram_sha_256_conv.password, "password")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.password to be 'password', got '%s'\n", scram_sha_256_conv.password);
        return 1;
    }

    if (strcmp(scram_sha_256_conv.mechanism_name, "SCRAM-SHA-256")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.mechanism_name to be 'SCRAM-SHA-256', got '%s'\n", scram_sha_256_conv.mechanism_name);
        return 1;
    }

    if (scram_sha_256_conv.status != CR_OK) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.status to be '%d', got '%d'\n", CR_OK, scram_sha_256_conv.status);
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}

