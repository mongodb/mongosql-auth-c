#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    cd $PROJECT_DIR

    echo "generating artifact tarball..."
    rm -rf $ARTIFACTS_DIR/mysql-server
    tar czf artifacts.tar.gz test/artifacts
    mv artifacts.tar.gz test/artifacts
    echo "done generating artifact tarball"

) > $LOG_FILE 2>&1

print_exit_msg
