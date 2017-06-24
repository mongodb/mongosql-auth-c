
case $VARIANT in
ubuntu1404-64)
    export CC='/opt/mongodbtoolchain/v2/bin/clang'
    export SQLPROXY_BUILD_VARIANT='full_matrix__os_full_matrix~ubuntu1404-64_mongodb_version~latest_mongodb_topology~standalone'
    CMAKE_PATH='/opt/cmake/bin'
    PUSH_ARCH='x86_64-ubuntu-1404'
    PUSH_NAME='linux'
    ;;
rhel70)
    export CC='/opt/mongodbtoolchain/v2/bin/clang'
    export SQLPROXY_BUILD_VARIANT='single_variant__os_single_variant~rhel70'
    CMAKE_PATH='/opt/cmake/bin'
    PUSH_ARCH='x86_64-rhel70'
    PUSH_NAME='linux'
    ;;
osx)
    export SQLPROXY_BUILD_VARIANT='full_matrix__os_full_matrix~osx_mongodb_version~latest_mongodb_topology~standalone'
    CMAKE_PATH='/Applications/Cmake.app/Contents/bin'
    PUSH_ARCH='x86_64'
    PUSH_NAME='osx'
    ;;
windows-vs2013)
    export SQLPROXY_BUILD_VARIANT='full_matrix__os_full_matrix~windows_mongodb_version~latest_mongodb_topology~standalone'
    CMAKE_PATH="/cygdrive/c/cmake/bin"
    CMAKE_GENERATOR="Visual Studio 12 2013 Win64"
    MSBUILD_PATH="/cygdrive/c/Program Files (x86)/MSBuild/12.0/Bin"
    PUSH_ARCH='x86_64'
    PUSH_NAME='win32'
    ;;
other) # on evergreen, but "variant" expansion not set
    ;;
esac
