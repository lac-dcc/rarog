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
HOISTED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_hoisted.mlir"

if ! [ -f $LINALG_MODEL ]
then
    echo $LINALG_MODEL not found, running create_linalg.sh
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

$MLIR_OPT \
    --one-shot-bufferize="bufferize-function-boundaries" \
    --buffer-deallocation-pipeline \
    $LINALG_MODEL -o "${RAROG_ROOT}/tmp/${MODEL_NAME}_buffered.mlir"

$RAROG_OPT_PATH \
    --hoist-dealloc \
    $LINALG_MODEL -o "${RAROG_ROOT}/tmp/${MODEL_NAME}_buffered_hoisted.mlir"

# Apply lowering pipeline
$RAROG_OPT_PATH \
    --rarog-lowering-pipeline \
    --instrument-malloc \
    $LINALG_MODEL -o $LOWERED_MODEL

# Apply lowering pipeline with dealloc hoisting
$RAROG_OPT_PATH \
    --rarog-lowering-pipeline="enable-hoist-dealloc" \
    --instrument-malloc \
    $LINALG_MODEL -o $HOISTED_MODEL

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" $MLIR_RUNNER \
    $LOWERED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered.txt 2> ${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.out

/usr/bin/time --format="\ntime elapsed: %es\nmax memory used: %Mkb\nCPU used: %P" $MLIR_RUNNER \
    $HOISTED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_hoisted.txt 2> ${RAROG_ROOT}/tmp/${MODEL_NAME}_hoisted.out

tail -n1 ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered.txt > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered_tailed.txt
tail -n1 ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_hoisted.txt > ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_hoisted_tailed.txt

diff ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered_tailed.txt ${RAROG_ROOT}/tmp/${MODEL_NAME}_output_hoisted_tailed.txt