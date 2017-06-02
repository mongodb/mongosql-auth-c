
set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)

IF(APPLE)
  MESSAGE(STATUS "Using Common Crypto")
  set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
  set (SSL_LIBS "-framework CoreFoundation -framework Security")
ELSEIF(WIN32)
  MESSAGE(STATUS "Using CNG")
  set (MONGOC_ENABLE_CRYPTO_CNG 1)
  set(SSL_LIBS crypt32.lib Bcrypt.lib)
ELSE()
  MESSAGE(STATUS "Using OpenSSL")
  set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
  include(FindOpenSSL)
  MESSAGE(STATUS "OPENSSL_FOUND: ${OPENSSL_FOUND}")
  MESSAGE(STATUS "OPENSSL_LIBRARIES: ${OPENSSL_LIBRARIES}")
  MESSAGE(STATUS "OPENSSL_INCLUDE_DIR: ${OPENSSL_INCLUDE_DIR}")
  include_directories(${OPENSSL_INCLUDE_DIR})
  set(SSL_LIBS ${OPENSSL_LIBRARIES})
ENDIF()

MYSQL_ADD_PLUGIN(mongosql_auth ../plugin/auth/mongosql-auth/mongosql-auth-plugin.c
  MODULE_ONLY TEST_ONLY)

configure_file (
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h.in"
  "${PROJECT_SOURCE_DIR}/plugin/auth/mongosql-auth/mongoc/mongoc-config.h"
)

add_library(mongoc
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
target_link_libraries(mongoc ${SSL_LIBS})

IF(UNIX)
  target_compile_options(mongoc PUBLIC "$<1:-fPIC>")
ENDIF()

target_link_libraries(mongosql_auth mongoc)
