#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    mongoBin=$ARTIFACTS_DIR/mongodb/bin/mongo

    if [ -z "$MECHANISM" ]; then
        MECHANISM="SCRAM-SHA-1"
    fi

    if [ ! -z "$USERNAME" ] && [ ! -z "$PASSWORD" ]; then
        createUserCmd="db.createUser({user: '$USERNAME', pwd: '$PASSWORD', roles: $roles})"
        if [ "$MECHANISM" == "SCRAM-SHA-256" ]; then
            createUserCmd="db.createUser({user: '$USERNAME', pwd: '$PASSWORD', roles: $MONGO_OTHER_USER_ROLES, mechanisms: [\"$MECHANISM\"]})"
        fi
        echo "Creating $MECHANISM user: $createUserCmd"
        $mongoBin $MONGO_CLIENT_ARGS admin --eval "$createUserCmd"
    else
        echo "Skipping secondary user creation"
    fi


) > $LOG_FILE 2>&1

print_exit_msg
