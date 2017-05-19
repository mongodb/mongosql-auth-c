# set ssl_libs based on platform
IF(APPLE)
  #set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 1)
  set (SSL_LIBS "-framework CoreFoundation -framework Security")
ELSEIF()
  include(FindOpenSSL)
  if(OPENSSL_FOUND)
    MESSAGE(STATUS ">>>>>> FOUND!!!")
  endif()
  set(SSL_LIBS ${OPENSSL_LIBRARIES})
ENDIF()

MYSQL_ADD_PLUGIN(mongosql_auth ../plugin/auth/mongosqld/auth_mongosql_plugin.c
  MODULE_ONLY TEST_ONLY)
