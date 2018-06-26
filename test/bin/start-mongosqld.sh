#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    echo "downloading mongosqld..."
    python $PROJECT_DIR/test/bin/download-mongosqld.py
    echo "done downloading mongosqld"

    echo "unpacking mongosqld..."
    cd $MONGOSQLD_DOWNLOAD_DIR
    if [ "Windows_NT" = "$OS" ]; then
        msiexec /i mongosqld /qn /log $ARTIFACTS_DIR/log/mongosql-install.log
        sleep 2
    else
        tar xzvf mongosqld
    fi
    echo "done unpacking mongosqld"

    if [ "Windows_NT" = "$OS" ]; then
        echo "stopping and deleting existing mongosql service..."
        net stop mongosql || true
        sc.exe delete mongosql || true
        reg delete "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\mongosql" /f || true
        echo "stopped and deleted existing mongosql service"
    fi

    echo "starting mongosqld..."
    if [ "Windows_NT" = "$OS" ]; then
        echo "  ...installing mongosql service"
        cd "/cygdrive/c/Program Files/MongoDB/Connector for BI/2.4/bin"
            ./mongosqld.exe install \
            $MONGO_URI \
            --logPath $ARTIFACTS_DIR/log/mongosqld.log \
            --schema $PROJECT_DIR/test/resources/mongosqld \
            --auth \
            -vv \
            --mongo-username $MONGO_USERNAME \
            --mongo-password $MONGO_PASSWORD
        echo "  ...starting mongosql service"
        net start mongosql
    else
        cd *
        cd bin
        nohup ./mongosqld \
            $MONGO_URI \
            --logPath $ARTIFACTS_DIR/log/mongosqld.log \
            --schema $PROJECT_DIR/test/resources/mongosqld \
            --auth \
            -vv \
            --mongo-username $MONGO_USERNAME \
            --mongo-password $MONGO_PASSWORD &
    fi
    echo "done starting mongosqld"
    sleep 5
# redirect stdin and stdout to a file instead of piping to tee so that we do
# not need to run this as a background task.
) > $LOG_FILE 2>&1

print_exit_msg
