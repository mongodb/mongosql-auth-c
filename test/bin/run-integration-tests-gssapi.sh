#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

test_connect() {
    cmd="$MYSQL \
        --host localhost \
        --protocol=TCP \
        --port 3307 \
        --plugin-dir=$ARTIFACTS_DIR/build/ \
        --default-auth=mongosql_auth \
        --password="$user_pass" \
        --user=$user_name?mechanism=GSSAPI"
    echo "select id from kerberos.test" | exec $cmd
}

(
    set -o errexit

    # This test depends on a make target of sqlproxy that includes a private keytab file,
    # so we must pull it and build manually.
    SQLPROXY_DIR="${ARTIFACTS_DIR}/go/src/github.com/10gen/sqlproxy"
    if [ ! -d $SQLPROXY_DIR ]; then
        echo "cloning sqlproxy repo"
        mkdir -p $SQLPROXY_DIR
        git clone git@github.com:10gen/sqlproxy.git ${SQLPROXY_DIR}
    fi
    cd $SQLPROXY_DIR 
    git fetch

    echo "building sqlproxy on variant $VARIANT"
    make clean build-mongosqld run-mongosqld-gssapi

    export KRB5_TRACE="${ARTIFACTS_DIR}/log/krb5.log"
    export KRB5_CONFIG="${PROJECT_DIR}/test/resources/gssapi/krb5.conf"

    user_name="${GSSAPI_USER}"
    principal="${GSSAPI_USER}@LDAPTEST.10GEN.CC"
    user_pass=""

    # Test auth after kinit
    # Cache a delegated credential from the default keytab
    kinit -k -t $GSSAPI_KTNAME -f $principal
    test_connect

    # Test providing password on the CLI
    kdestroy
    user_pass="$GSSAPI_PASSWD"
    test_connect

    # Test providing a client keytab with an environment variable.
    # This test does not work on MacOS Heimdal Kerberos.
    case $VARIANT in
    ubuntu1404*|rhel*)
        kdestroy
        export KRB5_CLIENT_KTNAME=$GSSAPI_KTNAME
        user_pass=""
        test_connect
        ;;
    *)
        echo "Skipping keytab test because variant is '$VARIANT'"
        ;;
    esac
) 2>&1 | tee $LOG_FILE

print_exit_msg

