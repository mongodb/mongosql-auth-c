#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    cd "$ARTIFACTS_DIR/log"
    ls -la

    echo "copying log files..."
    cp ./* $ARTIFACTS_DIR/out
    echo "done copying log files"

) > $LOG_FILE 2>&1

print_exit_msg
