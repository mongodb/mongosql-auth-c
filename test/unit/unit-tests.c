
#include <my_global.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "unit-tests.h"
#include "mongosql-auth.h"
#include "mongoc/mongoc-misc.h"
#include "mongoc/mongoc-scram.h"

int
main (int argc, char *argv[]) {

    int ret = 0;

    ret += test_mongosql_auth_defaults();

    return ret;
}

int test_mongosql_auth_defaults () {
    mongosql_auth_t plugin;

    fprintf(stderr, "Testing mongosql_auth_t constructor...");

    _mongosql_auth_init(&plugin, NULL, "username", "password");

    if (strcmp(plugin.username, "username")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected plugin.username to be 'username', got '%s'\n", plugin.username);
        return 1;
    }

    if (strcmp(plugin.password, "password")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected plugin.password to be 'password', got '%s'\n", plugin.password);
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}

int test_mongosql_auth_parameters () {
    mongosql_auth_t plugin;

    fprintf(stderr, "Testing mongosql_auth_t parameter removal...");

    _mongosql_auth_init(&plugin, NULL, "username?source=mydb", "password");

    if (strcmp(plugin.username, "username")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected plugin.username to be 'username', got '%s'\n", plugin.username);
        return 1;
    }

    if (strcmp(plugin.password, "password")) {
        fprintf(stderr, "FAIL\n");
        fprintf(stderr, "    expected plugin.password to be 'password', got '%s'\n", plugin.password);
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}

