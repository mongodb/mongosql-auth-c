set (MONGOC_ENABLE_CRYPTO_COMMON_CRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 0)
set (MONGOC_ENABLE_CRYPTO_CNG 0)

IF(APPLE)
    MESSAGE(STATUS "Using OpenSSL")
    set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
    set(MONGO_CRYPTO_LIBS "${WITH_SSL}/lib/libcrypto.1.0.0.dylib" "${WITH_SSL}/lib/libssl.1.0.0.dylib")
    include_directories("${WITH_SSL}/include")
    MESSAGE(STATUS "OpenSSL include path: ${WITH_SSL}")
    MESSAGE(STATUS "Using Common Crypto for mongoc crypto")
ELSEIF(WIN32)
    MESSAGE(STATUS "Using CNG for mongoc crypto")
    set (MONGOC_ENABLE_CRYPTO_CNG 1)
    set(MONGO_CRYPTO_LIBS crypt32.lib Bcrypt.lib)
ELSE()
    MESSAGE(STATUS "Using OpenSSL for mongoc crypto")
    set (MONGOC_ENABLE_CRYPTO_LIBCRYPTO 1)
    include(FindOpenSSL)
    if (OPENSSL_FOUND)
        set(MONGO_CRYPTO_LIBS ${OPENSSL_LIBRARIES})
        include_directories(${OPENSSL_INCLUDE_DIR})
        MESSAGE(STATUS "Found OpenSSL libraries")
        MESSAGE(STATUS "OpenSSL include path: ${OPENSSL_INCLUDE_DIR}")
    else()
        MESSAGE(FATAL_ERROR "Could not find OpenSSL libraries")
    endif()
ENDIF()

MESSAGE(STATUS "using mongoc crypto library: ${MONGO_CRYPTO_LIBS}")
