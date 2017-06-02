
#include <my_global.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "test-examples.h"
#include "mongoc/mongoc-misc.h"
#include "mongoc/mongoc-scram.h"

int
main (int argc, char *argv[]) {

    int ret = 0;
    ret += test_example_pass();
    ret += test_example_scram();

    return ret;
}

int
test_example_pass () {
    fprintf(stderr, "Running example unit test...");
    fprintf(stderr, "PASS\n");
    return 0;
}

int test_example_scram () {
    fprintf(stderr, "Testing mongoc scram basics...");

    mongoc_scram_t scram;
    _mongoc_scram_init (&scram);
    _mongoc_scram_set_user (&scram, "username");

    if (strcmp(scram.user, "username") != 0) {
        fprintf(stderr, "FAIL\n");
        return 1;
    }

    fprintf(stderr, "PASS\n");
    return 0;
}
