
. "$(dirname $0)/platforms.sh"

set -o pipefail

MAJOR_VERSION=1
MINOR_VERSION=3
PATCH_VERSION=0

export PATH="$PATH:$CMAKE_PATH:$MSBUILD_PATH"

basename=${0##*/}

SCRIPT_DIR="${SCRIPT_DIR:-$(cd "$(dirname $0)" && pwd -P)}"
if [ "Windows_NT" = "$OS" ]; then
    SCRIPT_DIR="$(cygpath -m $SCRIPT_DIR)"
fi

PROJECT_DIR="${PROJECT_DIR:-$(dirname "$(dirname $SCRIPT_DIR)")}"
if [ "Windows_NT" = "$OS" ]; then
    PROJECT_DIR="$(cygpath -m $PROJECT_DIR)"
fi

ARTIFACTS_DIR="$PROJECT_DIR/test/artifacts"
LOG_FILE="$ARTIFACTS_DIR/log/${basename%.sh}.log"
mkdir -p "$ARTIFACTS_DIR/log"
# boost variables
BOOST_DIR="boost_1_59_0"
BOOST_FILE="$BOOST_DIR.tar.gz"
FULL_BOOST_DIR_PATH="$ARTIFACTS_DIR/$BOOST_DIR"
BOOST_S3_URL="http://noexpire.s3.amazonaws.com/sqlproxy/sources/$BOOST_FILE"

export MONGOSQLD_DOWNLOAD_DIR="$ARTIFACTS_DIR/mongosqld"
export MONGO_ORCHESTRATION_HOME="$ARTIFACTS_DIR/orchestration"

CMAKE_ARGS="-DWITH_BOOST=$FULL_BOOST_DIR_PATH"
platform="$(uname)"
if [ "Linux" = "$platform" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DWITH_SSL=system"
fi

# icu variables
ICU_DIR="$ARTIFACTS_DIR/icu"
ICU_SRC_DIR="$ICU_DIR/icu/source"
ICU_BUILD_DIR="$ICU_DIR/build"
ENABLE_ICU='ON'
CMAKE_ICU_ARGS="-DENABLE_ICU=$ENABLE_ICU -DICU_ROOT=$ICU_BUILD_DIR -DICU_INCLUDE_DIR=$ICU_SRC_DIR/common"
CMAKE_ARGS="$CMAKE_ARGS $CMAKE_ICU_ARGS"

if [ "Windows_NT" = "$OS" ]; then
    bison_path="/cygdrive/c/bison/bin"
    BUILD='MSBuild.exe MySQL.sln'
    PLUGIN_LIBRARY="$ARTIFACTS_DIR/mysql-server/bld/Debug/mongosql_auth.dll"
    UNIT_TESTS="$ARTIFACTS_DIR/mysql-server/bld/Debug/mongosql_auth_unit_tests.exe"
    MYSQL="$ARTIFACTS_DIR/mysql-server/bld/client/Debug/mysql.exe"
    export PATH="$PATH:$bison_path"
else
    BUILD="make mongosql_auth mongosql_auth_so mongosql_auth_unit_tests mysql"
    PLUGIN_LIBRARY="$ARTIFACTS_DIR/mysql-server/bld/mongosql_auth.so"
    UNIT_TESTS="$ARTIFACTS_DIR/mysql-server/bld/mongosql_auth_unit_tests"
    MYSQL="$ARTIFACTS_DIR/mysql-server/bld/client/mysql"
fi

export AUTH=auth
MONGODB_BINARIES="$ARTIFACTS_DIR/mongodb/bin"
export PATH="$MONGODB_BINARIES:$PATH"

if [ "$RELEASE" == "" ]; then
    CMAKE_ARGS="$CMAKE_ARGS -DMONGOC_DEBUG=1"
else
    CMAKE_ARGS="$CMAKE_ARGS -DMONGOC_DEBUG=0"
    if [ "Windows_NT" = "$OS" ]; then
        BUILD="$BUILD /p:Configuration=Release"
        PLUGIN_LIBRARY="$ARTIFACTS_DIR/mysql-server/bld/Release/mongosql_auth.dll"
    else
        CMAKE_ARGS="$CMAKE_ARGS -DBUILD_CONFIG=mysql_release -DIGNORE_AIO_CHECK=1"
    fi
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
