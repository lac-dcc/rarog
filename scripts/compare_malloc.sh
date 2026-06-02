#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"

MLIR_OPT=${MLIR_OPT:-mlir-opt}
MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
INSTRUMENTED_MALLOC="${RAROG_ROOT}/utils/libinstrumented_malloc.so"
RAROG_MALLOC="${RAROG_ROOT}/utils/librarog_malloc.so"
MLIR_UTILS=${MLIR_UTILS:-/usr/lib/llvm/lib/libmlir_runner_utils.so}
MLIR_C_UTILS=${MLIR_C_UTILS:-/usr/lib/llvm/lib/libmlir_c_runner_utils.so}


# MODEL_PATH="${MODEL_PATH:-$RAROG_ROOT/onnx_models}"
MODEL_NAME="${MODEL_NAME:-model_1}"

if ! [ -f $RAROG_OPT_PATH ]
then
    # echo "rarog-opt is not compiled. Starting compilation process..."
    cd $RAROG_ROOT
    cmake -B build . --fresh
    cmake --build build
    if [[ $? != 0 ]]
    then
        echo "Compilation failed! Terminating..."
        exit 1
    fi
    cd -
fi

LINALG_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_linalg.mlir"
LOWERED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir"
STATIC_ALLOCATION_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.mlir"

if ! [ -f $LOWERED_MODEL ]
then
    bash "${RAROG_ROOT}/scripts/lower.sh" &> /dev/null
fi

if ! [ -f $STATIC_ALLOCATION_MODEL ]
then
    bash "${RAROG_ROOT}/scripts/lower_static_allocation.sh" &> /dev/null
fi

# TODO: Compare with the ILP allocation
/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" $MLIR_RUNNER \
    $LOWERED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered.txt 2> ${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.out

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" $MLIR_RUNNER \
    $STATIC_ALLOCATION_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$RAROG_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static_allocation.txt 2> ${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.out

tail -n1 ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered.txt > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered_tailed.txt
tail -n1 ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static_allocation.txt > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static_allocation_tailed.txt

diff ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered_tailed.txt ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static_allocation_tailed.txt