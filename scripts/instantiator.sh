#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"

MODEL_NAME="${MODEL_NAME:-model_1}"

if [[ ! -f $RAROG_OPT_PATH || $FRESH == true ]]; then
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
INSTRUMENTED_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.txt"
OUTPUT_FILE="${RAROG_ROOT}/memory_allocation_input/${MODEL_NAME}.in"

if ! [ -f $LINALG_MODEL ]
then
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

if ! [ -f $INSTRUMENTED_OUTPUT ]
then
    echo "Instrumentation not found, running instrumentation"
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh"
fi

$RAROG_OPT_PATH \
    --nasbench-lowering-pipeline="enable-reorder-frees" \
    --memory-allocation-instantiation="result-file=${INSTRUMENTED_OUTPUT}" \
    $LINALG_MODEL -o /dev/null > $OUTPUT_FILE