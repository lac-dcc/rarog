#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
INSTRUMENTED_MALLOC="${RAROG_ROOT}/utils/libinstrumented_malloc.so"
RAROG_MALLOC="${RAROG_ROOT}/utils/libstatic_malloc.so"
MLIR_UTILS=${MLIR_UTILS:-/usr/lib/llvm/lib/libmlir_runner_utils.so}
MLIR_C_UTILS=${MLIR_C_UTILS:-/usr/lib/llvm/lib/libmlir_c_runner_utils.so}

MODEL_NAME="${MODEL_NAME:-model_1}"

STATIC_ALLOCATION_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.mlir"
STATIC_ALLOCATION_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.out"

if ! [ -f $STATIC_ALLOCATION_MODEL ];
then
    bash "${RAROG_ROOT}/scripts/lower_static_allocation.sh" &> /dev/null
fi

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" $MLIR_RUNNER \
    $STATIC_ALLOCATION_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$RAROG_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS 2> $STATIC_ALLOCATION_OUTPUT