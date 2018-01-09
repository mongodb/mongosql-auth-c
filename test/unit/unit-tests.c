
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
    mongosql_auth_conversation_t conv;

    fprintf(stderr, "Testing mongosql_auth_conversation_t SCRAM-SHA-1 parameter initialization...");

    _mongosql_auth_conversation_init(&conv, "username?source=mydb", "password", "scram-sha-1", NULL);

    if (strcmp(conv.username, "username")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.username to be 'username', got '%s'\n", conv.username);
        return 1;
    }

    if (strcmp(conv.password, "password")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.password to be 'password', got '%s'\n", conv.password);
        return 1;
    }

    if (strcmp(conv.mechanism_name, "SCRAM-SHA-1")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.mechanism_name to be 'SCRAM-SHA-1', got '%s'\n", conv.mechanism_name);
        return 1;
    }

    if (conv.status != CR_OK) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected conv.status to be '%d', got '%d'\n", CR_OK, conv.status);
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}

