#!/bin/bash

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
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"
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

CLANG=${CLANG:-clang}
MLIR_TRANSLATE=${MLIR_TRANSLATE:-mlir-translate}
MLIR_LIBS=${MLIR_LIBS:-/usr/lib/llvm/lib}
RAROG_LIBS="${RAROG_ROOT}/utils"

MODEL_NAME="${MODEL_NAME:-model_1}"

STATIC_ALLOCATION_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.mlir"
LL_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation.ll"
BIN_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_allocation"

if ! [ -f $STATIC_ALLOCATION_MODEL ]
then
    bash "${RAROG_ROOT}/scripts/lower_static_allocation.sh" $ALLOCATION_HOISTING $DEALLOCATION_HOISTING $FRESH &> /dev/null
fi

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "${LL_FILE}.log" \
    $MLIR_TRANSLATE --mlir-to-llvmir $STATIC_ALLOCATION_MODEL -o $LL_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "${BIN_FILE}.log" $CLANG -fuse-ld=lld \
    -Wno-override-module \
    -Wl,-rpath,$MLIR_LIBS \
    -Wl,-rpath,$RAROG_LIBS \
    -L $MLIR_LIBS \
    -L $RAROG_LIBS \
    -lmlir_runner_utils \
    -lmlir_c_runner_utils \
    -linstrumented_malloc \
    -lstatic_malloc \
    -lm -o2 \
    $LL_FILE -o $BIN_FILE
