
set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)

IF(APPLE)
  MESSAGE(STATUS "Using Common Crypto")
  set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
  set (SSL_LIBS "-framework CoreFoundation -framework Security")
ELSE()
  MESSAGE(STATUS "Using OpenSSL")
  set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
  include(FindOpenSSL)
  MESSAGE(STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
  MESSAGE(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
  set(SSL_LIBS ${OPENSSL_LIBRARIES})
ENDIF()

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosqld/mongoc-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosqld/mongoc-config.h"
)

MYSQL_ADD_PLUGIN(mongosql_auth ../plugin/auth/mongosqld/auth_mongosql_plugin.c
  MODULE_ONLY TEST_ONLY)
