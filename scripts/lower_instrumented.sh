#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $LINALG_MODEL ] || $FRESH
then
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

$RAROG_OPT_PATH \
    --rarog-lowering-pipeline="$ALLOCATION_HOISTING $DEALLOCATION_HOISTING" \
    --instrument-malloc \
    $LINALG_MODEL -o $INSTRUMENTED_MODEL