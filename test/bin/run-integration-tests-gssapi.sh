#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

test_connect() {
    cmd="$MYSQL \
        --host localhost \
        --protocol=TCP \
        --port 3307 \
        --plugin-dir=$ARTIFACTS_DIR/build/ \
        --default-auth=mongosql_auth \
        --password=$password \
        --user=$principal?mechanism=GSSAPI&serviceName=mongosql2"
    echo "select _id from kerberos.test" | exec $cmd
}

(
    set -o errexit

    if [ "$GSSAPI_USER" = '' ]; then
        echo 'GSSAPI_USER environment variable not set'
        exit 1
    fi
    if [ "$GSSAPI_PASSWD" = '' ]; then
        echo 'GSSAPI_PASSWD environment variable not set'
        exit 1
    fi
    if [ "$GSSAPI_KTNAME" = '' ]; then
        echo 'GSSAPI_KTNAME environment variable not set'
        exit 1
    fi
    if [ ! -f "$GSSAPI_KTNAME" ]; then
        echo "could not find file specified in GSSAPI_KTNAME ($GSSAPI_KTNAME)"
        exit 1
    fi

    # This test depends on a make target of mongosqld that includes a private keytab file,
    # so we must pull it and build manually.
    MONGOSQLD_DIR="${ARTIFACTS_DIR}/go/src/github.com/10gen/sqlproxy"
    if [ ! -d $MONGOSQLD_DIR ]; then
        echo "cloning mongosqld repo"
        mkdir -p $MONGOSQLD_DIR
        git clone https://github.com/10gen/sqlproxy.git ${MONGOSQLD_DIR}
    fi
    cd $MONGOSQLD_DIR
    git fetch

    echo "building mongosqld on variant $VARIANT"
    export BUILD_TAGS="$BUILD_TAGS gssapi"
    make clean build-mongosqld run-mongosqld-gssapi-right-username-right-password

    export KRB5_TRACE="${ARTIFACTS_DIR}/log/krb5.log"
    export KRB5_CONFIG="${PROJECT_DIR}/test/resources/gssapi/krb5.conf"

    principal="${GSSAPI_USER}@LDAPTEST.10GEN.CC"
    password=""

    # Test auth after kinit
    # Cache a delegated credential from the default keytab
    echo "running test with credentials cache"
    kinit -k -t $GSSAPI_KTNAME -f $principal
    test_connect

    # Test providing password on the CLI
    kdestroy
    echo "running test with cli password"
    password="$GSSAPI_PASSWD"
    test_connect

    # Test providing a client keytab with an environment variable.
    # This test does not work on MacOS Heimdal Kerberos.
    case $VARIANT in
    ubuntu*|rhel*)
        kdestroy
        export KRB5_CLIENT_KTNAME=$GSSAPI_KTNAME
        password=""
        echo "running test with keytab environment variable"
        test_connect
        ;;
    *)
        echo "Skipping keytab test because variant is '$VARIANT'"
        ;;
    esac
) 2>&1 | tee $LOG_FILE

print_exit_msg

