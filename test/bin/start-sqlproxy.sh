#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    echo "downloading sqlproxy..."
    python $PROJECT_DIR/test/bin/download-sqlproxy.py
    echo "done downloading sqlproxy"

    echo "unpacking sqlproxy..."
    cd $SQLPROXY_DOWNLOAD_DIR
    if [ "Windows_NT" = "$OS" ]; then
        msiexec /i sqlproxy /qn /log $ARTIFACTS_DIR/log/mongosql-install.log
	sleep 2
    else
        tar xzvf sqlproxy
    fi
    echo "done unpacking sqlproxy"

    if [ "Windows_NT" = "$OS" ]; then
        echo "stopping and deleting existing mongosql service..."
        net stop mongosql || true
        sc.exe delete mongosql || true
        reg delete "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\mongosql" /f || true
        echo "stopped and deleted existing mongosql service"
    fi

    echo "starting sqlproxy..."
    if [ "Windows_NT" = "$OS" ]; then
        cd "/cygdrive/c/Program Files/MongoDB/Connector for BI/2.4/bin"
	    ./mongosqld.exe install \
            $SQLPROXY_MONGO_ARGS \
            --logPath $ARTIFACTS_DIR/log/sqlproxy.log \
            --schema $PROJECT_DIR/test/resources/sqlproxy \
            --auth -vvvv --mongo-username $MONGO_USERNAME --mongo-password $MONGO_PASSWORD
        net start mongosql
    else
        cd *
        cd bin
        nohup ./mongosqld -vvvv \
            $SQLPROXY_MONGO_ARGS \
            --logPath $ARTIFACTS_DIR/log/sqlproxy.log \
            --schema $PROJECT_DIR/test/resources/sqlproxy \
            --auth --mongo-username $MONGO_USERNAME --mongo-password $MONGO_PASSWORD &
    fi
    echo "done starting sqlproxy"
	sleep 5
# redirect stdin and stdout to a file instead of piping to tee so that we do
# not need to run this as a background task.
) > $LOG_FILE 2>&1

print_exit_msg
