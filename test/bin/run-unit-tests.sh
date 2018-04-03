#!/bin/bash

. "$(dirname $0)/prepare-shell.sh"

(
    set -o errexit
    eval $UNIT_TESTS
) 2>&1 | tee $LOG_FILE

print_exit_msg

