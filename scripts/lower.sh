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
FRESH=false

while getopts "adfh" OPTION; do
  case $OPTION in
    a)  ALLOCATION_HOISTING="enable-reorder-mallocs" ;;
    d)  DEALLOCATION_HOISTING="enable-reorder-frees" ;;
    f)  FRESH=true ;;
    h)  usage ;;
 esac
done

# MODEL_PATH="${MODEL_PATH:-$RAROG_ROOT/onnx_models}"
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

LINALG_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_linalg.mlir"
LOWERED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_lowered.mlir"

if ! [ -f $LINALG_MODEL ]
then
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

# Apply lowering pipeline
/usr/bin/time --format="time elapsed: %e\n" -o "${LOWERED_MODEL}.log" $RAROG_OPT_PATH \
    --rarog-lowering-pipeline="$ALLOCATION_HOISTING $DEALLOCATION_HOISTING" \
    $LINALG_MODEL -o $LOWERED_MODEL