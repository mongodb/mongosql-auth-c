
scripts_dir="$(cd "$(dirname $0)" && pwd -P)"
basename=${0##*/}

PROJECT_DIR="${PROJECT_DIR:-$(dirname "$(dirname $scripts_dir)")}"
ARTIFACTS_DIR="$PROJECT_DIR/test/artifacts"
LOG_FILE="$ARTIFACTS_DIR/log/${basename%.sh}.log"

CMAKE_ARGS="-DDOWNLOAD_BOOST=1 -DWITH_BOOST=$ARTIFACTS_DIR/boost"
platform="$(uname)"
if [ "Linux" = "$platform" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DWITH_SSL=system"
fi

if [ "Windows_NT" = "$OS" ]; then
    cmake_path='/cygdrive/c/cmake/bin'
    bison_path="/cygdrive/c/bison/bin"
    export PATH="$PATH:$cmake_path:$bison_path"
    BUILD='MSBuild.exe MySQL.sln'
    PLUGIN_LIBRARY="$ARTIFACTS_DIR/mysql-server/bld/Debug/mongosql_auth.dll"
    UNIT_TESTS="$ARTIFACTS_DIR/mysql-server/bld/Debug/mongosql_auth_unit_tests.exe"
else
    BUILD="make mongosql_auth mongosql_auth_unit_tests"
    PLUGIN_LIBRARY="$ARTIFACTS_DIR/mysql-server/bld/mongosql_auth.so"
    UNIT_TESTS="$ARTIFACTS_DIR/mysql-server/bld/mongosql_auth_unit_tests"
fi

print_exit_msg() {
    exit_code=$?
    if [ "$exit_code" != "0" ]; then
        status=FAILURE
    else
        status=SUCCESS
    fi

    echo "$status: $basename" 1>&2
    if [ "$status" = "FAILURE" ]; then
        echo "printing log from failed script:" 1>&2
        cat $LOG_FILE 1>&2
    fi

    return $exit_code
}
