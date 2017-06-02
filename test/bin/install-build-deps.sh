#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit

    if [ "Windows_NT" = "$OS" ]; then
        echo "installing build dependencies..."
        mkdir $ARTIFACTS_DIR/bison
        cd $ARTIFACTS_DIR/bison
        curl -O https://superb-sea2.dl.sourceforge.net/project/gnuwin32/bison/2.4.1/bison-2.4.1-setup.exe
        chmod +x bison-2.4.1-setup.exe
        ./bison-2.4.1-setup.exe /VERYSILENT /DIR='C:\bison'
        cp /cygdrive/c/Program\ Files\ \(x86\)/MSBuild/14.0/Bin/MSBuild.exe /cygdrive/c/bison/bin
        echo "done installing build dependencies"
    fi

) > $LOG_FILE 2>&1

print_exit_msg

