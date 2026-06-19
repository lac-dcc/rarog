#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $INSTRUMENTED_MODEL ] || $FRESH
then
    bash "${RAROG_ROOT}/scripts/lower_instrumented.sh" $@
fi

$MLIR_RUNNER $INSTRUMENTED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > /dev/null 2> $INSTRUMENTED_OUTPUT