#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $STATIC_ALLOCATION_MODEL ] || $FRESH
then
    bash "${RAROG_ROOT}/scripts/lower_static_allocation.sh" $@
fi

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" \
    $MLIR_RUNNER $STATIC_ALLOCATION_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$RAROG_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS 2> $STATIC_ALLOCATION_OUTPUT