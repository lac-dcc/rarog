#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"

MODEL_NAME="${MODEL_NAME:-model_1}"

if ! [ -f $RAROG_OPT_PATH ]; then
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
ILP_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}.in"

if ! [ -f $ILP_FILE ]; then
    $RAROG_OPT_PATH --memory-allocation-instantiation "$LINALG_MODEL" \
        -o /dev/null > "$ILP_FILE"
fi