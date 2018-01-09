
set (UNIT_TEST_SOURCE_FILES
    ../plugin/auth/mongosql-auth/unit-tests.c
)
add_executable(mongosql_auth_unit_tests ${UNIT_TEST_SOURCE_FILES} ${PLUGIN_SOURCE_FILES})
target_link_libraries(mongosql_auth_unit_tests mongoc ${SASL_LIBS})
