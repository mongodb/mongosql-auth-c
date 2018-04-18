#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    cmd="$MYSQL \
        --host 127.0.0.1 \
        --port 3307 \
        --plugin-dir=$ARTIFACTS_DIR/build/ \
        --default-auth=mongosql_auth \
        --user=$PLAIN_USER?mechanism=PLAIN \
        --password=$PLAIN_PASSWD \
        --execute=exit"
    exec $cmd

) > $LOG_FILE 2>&1

print_exit_msg

