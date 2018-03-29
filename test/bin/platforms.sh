
case $VARIANT in
ubuntu1404-64)
    export CC='/opt/mongodbtoolchain/v2/bin/clang'
    export SQLPROXY_BUILD_VARIANT='linux-x86_64-ubuntu-1404'
    CMAKE_PATH='/opt/cmake/bin'
    PUSH_ARCH='x86_64-ubuntu-1404'
    PUSH_NAME='linux'
    ;;
rhel70)
    export CC='/opt/mongodbtoolchain/v2/bin/clang'
    export SQLPROXY_BUILD_VARIANT='linux-x86_64-rhel70'
    CMAKE_PATH='/opt/cmake/bin'
    PUSH_ARCH='x86_64-rhel70'
    PUSH_NAME='linux'
    ;;
macos)
    export SQLPROXY_BUILD_VARIANT='osx-x86_64'
    CMAKE_PATH='/Applications/Cmake.app/Contents/bin'
    PUSH_ARCH='x86_64'
    PUSH_NAME='osx'
    ;;
windows-64)
    export SQLPROXY_BUILD_VARIANT='win32-x86_64'
    CMAKE_PATH='/cygdrive/c/cmake/bin'
    CMAKE_GENERATOR='Visual Studio 14 2015 Win64'
    MSBUILD_PATH='/cygdrive/c/Program Files (x86)/MSBuild/14.0/Bin'
    PROGRAM_FILES_FOLDER='ProgramFiles64Folder'
    PUSH_ARCH='x86_64'
    PUSH_NAME='win32'
    ;;
windows-32)
    export SQLPROXY_BUILD_VARIANT='win32-x86_64'
    CMAKE_PATH='/cygdrive/c/cmake/bin'
    CMAKE_GENERATOR='Visual Studio 14 2015'
    MSBUILD_PATH='/cygdrive/c/Program Files (x86)/MSBuild/14.0/Bin'
    PROGRAM_FILES_FOLDER='ProgramFilesFolder'
    PUSH_ARCH='x86'
    PUSH_NAME='win32'
    ;;
other) # on evergreen, but "variant" expansion not set
    ;;
esac
