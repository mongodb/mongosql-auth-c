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
    else
        tar xzvf sqlproxy
    fi
    echo "done unpacking sqlproxy"

    if [ "Windows_NT" = "$OS" ]; then
        echo "stopping and deleting existing sqlproxy service..."
        net stop mongosql || true
        sc.exe delete mongosql || true
        reg delete "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\EventLog\Application\mongosql" /f || true
        echo "stopped and deleted existing sqlproxy service"
    fi

    echo "starting sqlproxy..."
    if [ "Windows_NT" = "$OS" ]; then
        cd /cygdrive/c/Program\ Files/MongoDB/Connector\ for\ BI/2.2/bin
        ./mongosqld.exe install \
            --logPath $ARTIFACTS_DIR/log/sqlproxy.log \
            --schema $PROJECT_DIR/test/resources/sqlproxy \
            --auth -vvvv
        net start mongosql
    else
        cd *
        cd bin
        nohup ./mongosqld -vvvv \
            --logPath $ARTIFACTS_DIR/log/sqlproxy.log \
            --schema $PROJECT_DIR/test/resources/sqlproxy \
            --auth &
    fi
    sleep 5
    echo "done starting sqlproxy"

) > $LOG_FILE 2>&1

print_exit_msg
