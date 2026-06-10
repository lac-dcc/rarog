#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
INSTRUMENTED_MALLOC="${RAROG_ROOT}/utils/libinstrumented_malloc.so"
MLIR_UTILS=${MLIR_UTILS:-/usr/lib/llvm/lib/libmlir_runner_utils.so}
MLIR_C_UTILS=${MLIR_C_UTILS:-/usr/lib/llvm/lib/libmlir_c_runner_utils.so}

MODEL_NAME="${MODEL_NAME:-model_1}"

INSTRUMENTED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.mlir"
INSTRUMENTED_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.txt"

if ! [ -f $INSTRUMENTED_MODEL ]
then
    bash "${RAROG_ROOT}/scripts/lower_instrumented.sh" &> /dev/null
fi

time $MLIR_RUNNER \
    $INSTRUMENTED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > /dev/null 2> $INSTRUMENTED_OUTPUT