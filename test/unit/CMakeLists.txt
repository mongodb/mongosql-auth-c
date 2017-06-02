
add_executable(mongosql_auth_unit_tests
  ../plugin/auth/mongosql-auth/test-examples.c
)
target_link_libraries(mongosql_auth_unit_tests mongoc)
