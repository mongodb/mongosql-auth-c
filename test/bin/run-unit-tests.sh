#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    eval $UNIT_TESTS

) > $LOG_FILE 2>&1

print_exit_msg

