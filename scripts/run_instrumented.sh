#!/bin/bash

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

usage() {
    echo "Usage: $0 [-a] [-d] [-f] [-h]"
    echo ""
    echo "Options:"
    echo "  -a              Enable allocation hoisting"
    echo "  -d              Enable deallocation hoisting"
    echo "  -f              Force recompile of rarog-opt"
    echo "  -h              Show this help message"
    exit 1
}

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
ALLOCATION_HOISTING=""
DEALLOCATION_HOISTING=""
FRESH=""

while getopts "adfh" OPTION; do
  case $OPTION in
    a)  ALLOCATION_HOISTING="-a" ;;
    d)  DEALLOCATION_HOISTING="-d" ;;
    f)  FRESH="-f" ;;
    h)  usage ;;
 esac
done

MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
INSTRUMENTED_MALLOC="${RAROG_ROOT}/utils/libinstrumented_malloc.so"
MLIR_UTILS=${MLIR_UTILS:-/usr/lib/llvm/lib/libmlir_runner_utils.so}
MLIR_C_UTILS=${MLIR_C_UTILS:-/usr/lib/llvm/lib/libmlir_c_runner_utils.so}

MODEL_NAME="${MODEL_NAME:-model_1}"

if ! [ -f $RAROG_OPT_PATH ] || $FRESH
then
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

INSTRUMENTED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.mlir"
INSTRUMENTED_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}.out"

if ! [ -f $INSTRUMENTED_MODEL ]
then
    bash "${RAROG_ROOT}/scripts/lower_instrumented.sh" $ALLOCATION_HOISTING $DEALLOCATION_HOISTING &> /dev/null
fi

time $MLIR_RUNNER \
    $INSTRUMENTED_MODEL \
    --entry-point-result=void \
    --shared-libs=$INSTRUMENTED_MALLOC \
    --shared-libs=$MLIR_UTILS \
    --shared-libs=$MLIR_C_UTILS > /dev/null 2> $INSTRUMENTED_OUTPUT