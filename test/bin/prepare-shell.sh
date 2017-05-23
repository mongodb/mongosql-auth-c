
scripts_dir="$(cd "$(dirname $0)" && pwd -P)"
PROJECT_DIR="${PROJECT_DIR:-$(dirname "$(dirname $scripts_dir)")}"
ARTIFACTS_DIR="$PROJECT_DIR/test/artifacts"

if [ "$ON_EVERGREEN" != "true" ]; then
    CURRENT_VERSION="$(git describe --always)"
    GIT_SPEC="$(git rev-parse HEAD)"
    PUSH_ARCH="x86_64-ubuntu1404"
    PUSH_NAME="linux"
fi

basename=${0##*/}
LOG_FILE="$ARTIFACTS_DIR/log/${basename%.sh}.log"

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
