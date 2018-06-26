# Defaults.
set (MONGOC_DEBUG 0)
set (MONGOC_ENABLE_ICU 0)
set (ENABLE_ICU OFF)

include(FindICU)
if (ICU_FOUND)
  message (STATUS "ICU was found, version is ${ICU_VERSION}")
  message (STATUS "Using ICU libraries from: ${ICU_LIBRARIES} with include dirs: ${ICU_INCLUDE_DIR}")
  set (MONGOC_ENABLE_ICU 1)
  include_directories ("${ICU_INCLUDE_DIR}")
elseif (ENABLE_ICU STREQUAL ON)
  message (FATAL_ERROR "No ICU library found. If ICU is installed in a non-standard directory, define ICU_ROOT as the ICU installation path.")
elseif (ENABLE_ICU STREQUAL AUTO)
  message (STATUS "No ICU library found, SASLPrep disabled for SCRAM-SHA-256 authentication.")
  message (STATUS "If ICU is installed in a non-standard directory, define ICU_ROOT as the ICU installation path.")
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

if (NOT ENABLE_ICU MATCHES "AUTO|ON|OFF")
   message (FATAL_ERROR, "ENABLE_ICU option must be AUTO, ON, or OFF")
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

add_library(mongoc STATIC ${MONGOC_SOURCE_FILES})
target_link_libraries(mongoc ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES})

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

MYSQL_ADD_PLUGIN(
    mongosql_auth_so
    ${PLUGIN_SOURCE_FILES}
    LINK_LIBRARIES ${MONGO_CRYPTO_LIBS} ${MONGO_KRB_LIBS} ${ICU_LIBRARIES}
    MODULE_ONLY
    MODULE_OUTPUT_NAME mongosql_auth
)
