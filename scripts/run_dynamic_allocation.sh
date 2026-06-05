#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $DYNAMIC_MODEL ] || $FRESH
then
    bash "${RAROG_ROOT}/scripts/lower_dynamic_allocation.sh" $@
fi

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" \
    $MLIR_RUNNER $DYNAMIC_MODEL \
    --entry-point-result=void \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS