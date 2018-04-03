include(CheckSymbolExists)

message (STATUS "Searching for gssapi/gssapi.h and gssapi/krb5.h")
find_path (
    GSS_INCLUDE_DIRS NAMES gssapi/gssapi.h
    PATHS /include /usr/include /usr/local/include /usr/share/include /opt/include c:/gssapi/include)

if (GSS_INCLUDE_DIRS)
    message (STATUS "  Found in ${GSS_INCLUDE_DIRS}")
else ()
    message (STATUS "  Not found (specify -DCMAKE_INCLUDE_PATH=C:/path/to/gssapi/include for GSSAPI support)")
endif ()

message (STATUS "Searching for gssapi_krb5")
find_library(
    GSS_LIBS NAMES gssapi_krb5
    PATHS /usr/lib/x86_64-linux-gnu /usr/lib /lib /usr/local/lib /usr/share/lib /opt/lib /opt/share/lib /var/lib c:/gssapi/lib)

if (GSS_LIBS)
    message (STATUS " Found ${GSS_LIBS}")
else ()
    message (STATUS "  Not found (specify -DCMAKE_LIBRARY_PATH=C:/path/to/gssapi/lib for GSSAPI support)")
endif ()

if (GSS_INCLUDE_DIRS AND GSS_LIBS)
    set (GSS_FOUND 1)
else ()
    set (GSS_FOUND 0)
endif ()
