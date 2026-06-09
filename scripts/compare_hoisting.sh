#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/../.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi


MLIR_OPT=${MLIR_OPT:-mlir-opt}
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"
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

BASE_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_base"
ALLOC_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_alloc"
DEALLOC_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_dealloc"
ALLOC_DEALLOC_BIN="${RAROG_ROOT}/tmp/${MODEL_NAME}_alloc_dealloc"

if ! [ -f $BASE_BIN ]
then
    echo "Compiling base binary"
    bash "${RAROG_ROOT}/scripts/compile.sh" &> /dev/null
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh" &> /dev/null
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.mlir
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.ll
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered" "${BASE_BIN}"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir.log" "${BASE_BIN}.mlir.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.ll.log" "${BASE_BIN}.ll.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.log" "${BASE_BIN}.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}.out" "${BASE_BIN}.out"
fi

if ! [ -f $ALLOC_BIN ]
then
    echo "Compiling alloc binary"
    bash "${RAROG_ROOT}/scripts/compile.sh" -a &> /dev/null
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh" -a &> /dev/null
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.mlir
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.ll
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered" "${ALLOC_BIN}"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir.log" "${ALLOC_BIN}.mlir.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.ll.log" "${ALLOC_BIN}.ll.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.log" "${ALLOC_BIN}.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}.out" "${ALLOC_BIN}.out"
fi

if ! [ -f $DEALLOC_BIN ]
then
    echo "Compiling dealloc binary"
    bash "${RAROG_ROOT}/scripts/compile.sh" -d &> /dev/null
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh" -d &> /dev/null
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.mlir
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.ll
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered" "${DEALLOC_BIN}"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir.log" "${DEALLOC_BIN}.mlir.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.ll.log" "${DEALLOC_BIN}.ll.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.log" "${DEALLOC_BIN}.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}.out" "${DEALLOC_BIN}.out"
fi

if ! [ -f $ALLOC_DEALLOC_BIN ]
then
    echo "Compiling alloc + dealloc binary"
    bash "${RAROG_ROOT}/scripts/compile.sh" -ad &> /dev/null
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh" -ad &> /dev/null
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.mlir
    rm "${RAROG_ROOT}/tmp/${MODEL_NAME}"*.ll
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered" "${ALLOC_DEALLOC_BIN}"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir.log" "${ALLOC_DEALLOC_BIN}.mlir.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.ll.log" "${ALLOC_DEALLOC_BIN}.ll.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.log" "${ALLOC_DEALLOC_BIN}.log"
    mv "${RAROG_ROOT}/tmp/${MODEL_NAME}.out" "${ALLOC_DEALLOC_BIN}.out"
fi

BASE_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_base.txt"
BASE_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_base_tailed.txt"

ALLOC_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_alloc.txt"
ALLOC_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_alloc_tailed.txt"

DEALLOC_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_dealloc.txt"
DEALLOC_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_dealloc_tailed.txt"

ALLOC_DEALLOC_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_alloc_dealloc.txt"
ALLOC_DEALLOC_FILE_TAILED="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_alloc_dealloc_tailed.txt"

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${BASE_BIN}.exe.log" $BASE_BIN > $BASE_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${ALLOC_BIN}.exe.log" $ALLOC_BIN > $ALLOC_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${DEALLOC_BIN}.exe.log" $DEALLOC_BIN > $DEALLOC_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "${ALLOC_DEALLOC_BIN}.exe.log" $ALLOC_DEALLOC_BIN > $ALLOC_DEALLOC_FILE

tail -n1 $BASE_FILE > $BASE_FILE_TAILED
tail -n1 $ALLOC_FILE > $ALLOC_FILE_TAILED
tail -n1 $DEALLOC_FILE > $DEALLOC_FILE_TAILED
tail -n1 $ALLOC_DEALLOC_FILE > $ALLOC_DEALLOC_FILE_TAILED

DELETE_BASE_FILE=true

diff $BASE_FILE_TAILED $ALLOC_FILE_TAILED
if [[ $? = 0 ]]
then
    rm $ALLOC_FILE
    rm $ALLOC_FILE_TAILED
else
    echo "Alloc output differs from base output"
    DELETE_BASE_FILE=false
fi

diff $BASE_FILE_TAILED $DEALLOC_FILE_TAILED
if [[ $? = 0 ]]
then
    rm $DEALLOC_FILE
    rm $DEALLOC_FILE_TAILED
else
    echo "Dealloc output differs from base output"
    DELETE_BASE_FILE=false
fi

diff $BASE_FILE_TAILED $ALLOC_DEALLOC_FILE_TAILED
if [[ $? = 0 ]]
then
    rm $ALLOC_DEALLOC_FILE
    rm $ALLOC_DEALLOC_FILE_TAILED
else
    echo "Alloc + dealloc output differs from base output"
    DELETE_BASE_FILE=false
fi

if $DELETE_BASE_FILE
then
    rm $BASE_FILE
    rm $BASE_FILE_TAILED
else
    exit 1
fi