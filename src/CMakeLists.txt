
# Defaults.
set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)
set (MONGOSQL_AUTH_ENABLE_SASL 0)
set (MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 0)
set (MONGOSQL_AUTH_ENABLE_SASL_SSPI 0)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${PROJECT_SOURCE_DIR}/cmake)
set (CMAKE_C_FLAGS "-std=c99 ${CMAKE_C_FLAGS}")

set (PLUGIN_SOURCE_FILES
    ../plugin/auth/mongosql-auth/mongosql-auth-plugin.c
    ../plugin/auth/mongosql-auth/mongosql-auth.c
    ../plugin/auth/mongosql-auth/mongosql-auth-conversation.c
)

IF(APPLE)
  MESSAGE(STATUS "Using Common Crypto")
  set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
  set (SSL_LIBS "-framework CoreFoundation -framework Security")

  MESSAGE(STATUS "Using GSSAPI")
  set(MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 1)
  set(SASL_LIBS "-framework GSS")
  set(PLUGIN_SOURCE_FILES ${PLUGIN_SOURCE_FILES} ../plugin/auth/mongosql-auth/mongosql-auth-gssapi.c)

  set(MONGOSQL_AUTH_ENABLE_SASL 1)
  set(MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 1)
ELSEIF(WIN32)
  MESSAGE(STATUS "Using CNG")
  set (MONGOC_ENABLE_CRYPTO_CNG 1)
  set(SSL_LIBS crypt32.lib Bcrypt.lib)
  set(SASL_LIBS secur32.lib Shlwapi.lib)

  MESSAGE(STATUS "Using SSPI")
  set(PLUGIN_SOURCE_FILES ${PLUGIN_SOURCE_FILES} ../plugin/auth/mongosql-auth/mongosql-auth-sspi.c)
  set(MONGOSQL_AUTH_ENABLE_SASL 1)
  set(MONGOSQL_AUTH_ENABLE_SASL_SSPI 1)
ELSE()
  MESSAGE(STATUS "Using OpenSSL")
  set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
  include(FindOpenSSL)
  MESSAGE(STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
  MESSAGE(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
  MESSAGE(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
  include_directories(${OPENSSL_INCLUDE_DIR})
  set(SSL_LIBS ${OPENSSL_LIBRARIES})

  include(FindGSSAPI)

  if (GSS_FOUND)
    set(SASL_LIBS ${GSS_LIBS})
    include_directories(${GSS_INCLUDE_DIRS})
    set(PLUGIN_SOURCE_FILES ${PLUGIN_SOURCE_FILES} ../plugin/auth/mongosql-auth/mongosql-auth-gssapi.c)
    set(MONGOSQL_AUTH_ENABLE_SASL 1)
    set(MONGOSQL_AUTH_ENABLE_SASL_GSSAPI 1)
    MESSAGE(STATUS "Using GSSAPI")
    MESSAGE(STATUS " -- GSSAPI include path: ${GSS_INCLUDE_DIRS}")
    MESSAGE(STATUS " -- GSSAPI library: ${SASL_LIBS}")
  else()
    MESSAGE(FATAL_ERROR "Could not find GSS libraries")
  endif()
ENDIF()


MYSQL_ADD_PLUGIN(mongosql_auth ${PLUGIN_SOURCE_FILES} LINK_LIBRARIES ${SASL_LIBS} MODULE_ONLY TEST_ONLY)

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h"
)

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongosql-auth-config.h"
)

set (MONGOC_SOURCE_FILES
    ../plugin/auth/mongosql-auth/mongoc/bson-md5.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-misc.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-b64.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-crypto-cng.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-crypto-common-crypto.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-crypto-openssl.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-crypto.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-memcmp.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-rand-cng.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-rand-common-crypto.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-rand-openssl.c
    ../plugin/auth/mongosql-auth/mongoc/mongoc-scram.c
)

add_library(mongoc ${MONGOC_SOURCE_FILES})
target_link_libraries(mongoc ${SSL_LIBS})

IF(UNIX)
  target_compile_options(mongoc PUBLIC "$<1:-fPIC>")
ENDIF()

target_link_libraries(mongosql_auth mongoc)
