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

DYNAMIC_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered"
STATIC_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation"

if ! [ -f $DYNAMIC_BIN ]
then
    echo "Compiling dynamic binary"
    bash "${RAROG_ROOT}/scripts/compile.sh" -ad # &> /dev/null
fi

if ! [ -f $STATIC_BIN ]
then
    echo "Compiling static binary"
    bash "${RAROG_ROOT}/scripts/compile_static_allocation.sh" -ad # &> /dev/null
fi

DYNAMIC_OUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.out"
DYNAMIC_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered.txt"
DYNAMIC_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_lowered_tailed.txt"

STATIC_OUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.out"
STATIC_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static.txt"
STATIC_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static_tailed.txt"

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${DYNAMIC_BIN}.exe.log" $DYNAMIC_BIN > $DYNAMIC_FILE 2> $DYNAMIC_OUT

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${STATIC_BIN}.exe.log" $STATIC_BIN > $STATIC_FILE 2> $STATIC_OUT

tail -n1 $DYNAMIC_FILE > $DYNAMIC_FILE_TAILED
tail -n1 $STATIC_FILE > $STATIC_FILE_TAILED

diff $DYNAMIC_FILE_TAILED $STATIC_FILE_TAILED
if [[ $? = 0 ]]
then
    rm $DYNAMIC_FILE
    rm $DYNAMIC_FILE_TAILED
    rm $STATIC_FILE
    rm $STATIC_FILE_TAILED
else
    echo "Static output differs from dynamic output"
fi