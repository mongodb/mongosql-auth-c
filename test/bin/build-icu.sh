#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

# We need this since we can't use errexit for configure and make.
fail_on_error() {
    if [ $1 -ne 0 ]; then
        >&2 echo "$2 failed with code $1."
        exit $1
    fi
}

(
    set -o errexit

    icu_archive="$ICU_DIR/icu.tgz"

    if [ ! -e "$ICU_SRC_DIR" ]; then
        echo 'no icu source found, need to download and extract'

        echo 'cleaning up existing icu build artifacts...'
        rm -rf "$ICU_DIR"
        mkdir "$ICU_DIR"
        rm -rf "$ICU_BUILD_DIR"
        mkdir "$ICU_BUILD_DIR"
        echo 'done cleaning up existing icu build artifacts'

        echo 'downloading icu...'
        cd "$ICU_DIR"
        curl 'https://sourceforge.net/projects/icu/files/ICU4C/62.1/icu4c-62_1-src.tgz/download' --output "$icu_archive"

        echo 'extracting icu...'
        if [ "Windows_NT" = "$OS" ]; then
                tar xzvf "$(cygpath -u $icu_archive)"
        else
                tar xzvf "$icu_archive"
        fi
        echo 'downloaded and extracted icu'
    else
        echo 'existing icu source found, no need to download'
        echo 'cleaning up existing icu build dir...'
        rm -rf "$ICU_BUILD_DIR"
        mkdir "$ICU_BUILD_DIR"
        echo 'cleaned up existing icu build dir'
    fi

    echo "configuring icu for platform '$ICU_PLATFORM'"
    # We need to use a CPPFLAG since this autoconf script does not support the usual --with-pic.
    CONFIGURE="CPPFLAGS=-fPIC $ICU_SRC_DIR/runConfigureICU $ICU_PLATFORM --enable-static --disable-shared"
    # configure will run things that can fail so we don't want it to inherit this.
    set +o errexit
    case $VARIANT in
    windows-64)
        # The cmd business brings all the needed compiler binaries into the shell's environment variables.
        cmd /c "C:\Program^ Files^ ^(x86^)\Microsoft^ Visual^ Studio^ 14.0\VC\vcvarsall.bat amd64 && bash -c 'cd $ICU_BUILD_DIR ^&^& eval $CONFIGURE'"
        fail_on_error $? configure
        cmd /c "C:\Program^ Files^ ^(x86^)\Microsoft^ Visual^ Studio^ 14.0\VC\vcvarsall.bat amd64 && bash -c 'cd $ICU_BUILD_DIR ^&^& make'"
        fail_on_error $? make
        ;;
    windows-32)
        # The cmd business brings all the needed compiler binaries into the shell's environment variables.
        cmd /c "C:\Program^ Files^ ^(x86^)\Microsoft^ Visual^ Studio^ 14.0\VC\vcvarsall.bat x86 && bash -c 'cd $ICU_BUILD_DIR ^&^& eval $CONFIGURE'"
        fail_on_error $? configure
        cmd /c "C:\Program^ Files^ ^(x86^)\Microsoft^ Visual^ Studio^ 14.0\VC\vcvarsall.bat x86 && bash -c 'cd $ICU_BUILD_DIR ^&^& make'"
        fail_on_error $? make
        ;;
    *)
        cd "$ICU_BUILD_DIR" && eval $CONFIGURE
        fail_on_error $? configure
        cd "$ICU_BUILD_DIR" && make
        fail_on_error $? make
        ;;
    esac
    set -o errexit

) > $LOG_FILE 2>&1

print_exit_msg
