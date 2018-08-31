# Defaults.
set (MONGOC_DEBUG 0)
set (MONGOC_ENABLE_ICU 0)

include(CheckCXXSourceCompiles)

# The order here is significant and specific to linking static ICU.
# Work must be done to ensure a dynamic build works.
find_package(ICU REQUIRED COMPONENTS uc i18n data)
if (ICU_FOUND)
  message (STATUS "ICU was found, version is ${ICU_VERSION}")
  message (STATUS "Using ICU libraries from: ${ICU_LIBRARIES} with include dirs: ${ICU_INCLUDE_DIR}")
  set (MONGOC_ENABLE_ICU 1)
  include_directories ("${ICU_INCLUDE_DIR}")
elseif (ENABLE_ICU STREQUAL ON)
  message (FATAL_ERROR "No ICU library found. If ICU is installed in a non-standard directory, define ICU_ROOT as the ICU installation path.")
endif()

include(FindMongoCrypto)
include(FindMongoKerberos)

set (CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set (CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

set (PLUGIN_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-plugin.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-conversation.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/bson-md5.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-misc.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-b64.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-cng.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-common-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-openssl.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-memcmp.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-cng.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-common-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-openssl.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-scram.c
)

IF(WIN32)
    set(PLUGIN_SOURCE_FILES ${PLUGIN_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-sspi.c)
ELSE()
    set(PLUGIN_SOURCE_FILES ${PLUGIN_SOURCE_FILES} ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-gssapi.c)
ENDIF()

if (NOT ENABLE_ICU MATCHES "ON|OFF")
   message (FATAL_ERROR, "ENABLE_ICU option must be ON or OFF")
endif()

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h"
)

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-config.h"
)

set (MONGOC_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/bson-md5.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-misc.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-b64.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-cng.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-common-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto-openssl.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-memcmp.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-cng.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-common-crypto.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-rand-openssl.c
    ${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-scram.c
)

# For now, we use "libstdc++" on Linux and "libc++" on OS X.
# This invasive check is neccesary since we can only check for OSX/Linux or gcc/clang and it is possible
# both to use libstdc++ on OSX or with clang and to use libc++ on Linux or with gcc.
# If "__GLIBCXX__" is defined, it means "libstdc++" is the library we're linking against
# (since it's only defined in a "libstdc++" header file).
CHECK_CXX_SOURCE_COMPILES(
"#include <iostream>
int main() {
#ifdef __GLIBCXX__
return 0;
#else
not libstdc++
#endif
}"
STD_CXX)

add_library(mongoc STATIC ${MONGOC_SOURCE_FILES})
IF(UNIX)
    if (STD_CXX)
        target_link_libraries(mongoc ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES} dl stdc++ m)
    else (STD_CXX)
        target_link_libraries(mongoc ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES} dl c++ m)
    endif (STD_CXX)
ELSE(UNIX)
    target_link_libraries(mongoc ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES})
ENDIF(UNIX)

IF(UNIX)
  target_compile_options(mongoc PUBLIC "$<1:-fPIC>")
ENDIF()

# We need two plugins because this is the only way to build the plugin both statically and dynamically.
# We need the static plugin for building mongo odbc, and we need the dynamic for stand-alone shipping
# of mongosql-auth-c.
MYSQL_ADD_PLUGIN(
    mongosql_auth
    ${PLUGIN_SOURCE_FILES}
    LINK_LIBRARIES ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES}
    MANDATORY
)

IF(UNIX)
    target_compile_options(mongosql_auth PUBLIC "$<1:-fPIC>")
ENDIF()

if (UNIX AND NOT STD_CXX)
MYSQL_ADD_PLUGIN(
    mongosql_auth_so
    ${PLUGIN_SOURCE_FILES}
    LINK_LIBRARIES ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES} c++
    MODULE_ONLY
    MODULE_OUTPUT_NAME mongosql_auth
)
else (UNIX AND NOT STD_CXX)
MYSQL_ADD_PLUGIN(
    mongosql_auth_so
    ${PLUGIN_SOURCE_FILES}
    LINK_LIBRARIES ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES}
    MODULE_ONLY
    MODULE_OUTPUT_NAME mongosql_auth
)
endif (UNIX AND NOT STD_CXX)

set_target_properties(mongosql_auth_so PROPERTIES COMPILE_FLAGS "-DU_STATIC_IMPLEMENTATION")
