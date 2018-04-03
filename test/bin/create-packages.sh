#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    echo "creating release..."

    cd $PROJECT_DIR

    build_dir="$ARTIFACTS_DIR/build"
    release_dir="mongosql-auth-$PUSH_NAME-$PUSH_ARCH"

    if [ "Windows_NT" = "$OS" ]; then

        # package the zip file
        python test/bin/make_archive.py \
            -o $ARTIFACTS_DIR/release.zip \
            --format zip \
            --transform $build_dir/mongosql_auth.dll=$release_dir/lib/mongosql_auth.dll \
            --transform README.md=$release_dir/README \
            --transform THIRD_PARTY_NOTICES=$release_dir/THIRD-PARTY-NOTICES \
            --transform LICENSE=$release_dir/LICENSE \
            LICENSE \
            README.md \
            THIRD_PARTY_NOTICES \
            $build_dir\\mongosql_auth.dll

        # build the msi. Since this is windows only, we know powershell is installed.
        SEMVER=$(git describe --always --abbrev=0)
        SEMVER=1.0.0
        powershell.exe \
            -NoProfile \
            -NoLogo \
            -NonInteractive \
            -File "$(cygpath -C ANSI -w "$SCRIPT_DIR/build-msi.ps1")" \
            -ProgramFilesFolder "$PROGRAM_FILES_FOLDER" \
            -ProjectName "MongoSQL Auth Plugin" \
            -VersionLabel "$SEMVER" \
            -WixPath "$WIX\\bin"

    else

        # package the tarball
        python test/bin/make_archive.py \
            -o $ARTIFACTS_DIR/release.tgz \
            --format tgz \
            --transform $build_dir/mongosql_auth.so=$release_dir/lib/mongosql_auth.so \
            --transform README.md=$release_dir/README \
            --transform THIRD_PARTY_NOTICES=$release_dir/THIRD-PARTY-NOTICES \
            --transform LICENSE=$release_dir/LICENSE \
            LICENSE \
            README.md \
            THIRD_PARTY_NOTICES \
            $build_dir/mongosql_auth.so

    fi

    echo "done creating release"
) 2>&1 | tee $LOG_FILE

print_exit_msg
