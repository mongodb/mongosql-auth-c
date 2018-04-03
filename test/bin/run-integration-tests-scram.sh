#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    cmd="$MYSQL
        --host 127.0.0.1 \
        --port 3307 \
        --plugin-dir=$ARTIFACTS_DIR/build/ \
        --default-auth=mongosql_auth \
        --user=bob \
        --password=pwd123 \
        --execute=exit"
    exec $cmd
) 2>&1 | tee $LOG_FILE

print_exit_msg

