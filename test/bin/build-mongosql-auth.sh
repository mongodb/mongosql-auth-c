#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    if [ "$1" = "--no-clobber" ]; then
        CLOBBER="false"
    else
        CLOBBER="true"
    fi

    if [ "$CLOBBER" = "true" ]; then
        echo "cloning mysql-server..."
        # clone mysql-server and checkout the right commit
        cd $ARTIFACTS_DIR
        git clone https://github.com/mysql/mysql-server
        cd mysql-server
        echo "done cloning mysql-server"
    else
        cd $ARTIFACTS_DIR/mysql-server/
    fi

    git checkout 4826e15 -- .

    # move source and build files into mysql repo
    rm -rf plugin/auth/mongosqld
    mkdir -p plugin/auth/mongosqld
    cp $PROJECT_DIR/src/mongosql-auth/*.{c,h,h.in} plugin/auth/mongosqld
    cat $PROJECT_DIR/src/CMakeLists.txt >> CMakeLists.txt

    # build the plugin
    echo "building mongosql_auth..."
    mkdir -p bld
    cd bld
    cmake .. -DDOWNLOAD_BOOST=1 -DWITH_BOOST=./boost
    make mongosql_auth
    mv mongosql_auth.so $ARTIFACTS_DIR/out
    echo "done building mongosql_auth"

) > $LOG_FILE 2>&1

print_exit_msg

