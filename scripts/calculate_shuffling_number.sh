#!/bin/bash

usage() {
    echo "Usage: $0 [-v] [-m model_index] [-f] [-h]"
    echo ""
    echo "Options:"
    echo "  -v              Enable verbose output"
    echo "  -m model_index  Select model from 'tmp/model_<MODEL_IDX>_linalg.mlir'"
    echo "  -f              Force recompile of rarog-opt"
    echo "  -h              Show this help message"
    exit 1
}

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"
VERBOSE=false
FRESH=false
MODEL_IDX=1

while getopts "vm:fh" OPTION; do
  case $OPTION in
    v)  VERBOSE=true ;;
    m)  MODEL_IDX=$OPTARG ;;
    f)  FRESH=true ;;
    h)  usage ;;
 esac
done

if [ "$VERBOSE" = true ]; then
    echo "Verbose: ON"
    echo "Model Index: ${MODEL_IDX}"
    echo "Force Recompile: ${FRESH}"
    echo ""
fi

print_blue() {
    $VERBOSE && echo -e "\e[0;34m * ${1}\e[0m"
}
print_red() {
    $VERBOSE && echo -e "\e[0;31m ! ${1}\e[0m"
}

if [[ ! -f $RAROG_OPT_PATH || $FRESH == true ]]; then
    print_blue "Compiling rarog-opt..."
    cd $RAROG_ROOT
    cmake -B build . --fresh
    cmake --build build
    if [[ $? != 0 ]]; then
        print_red "Compilation failed! Terminating..."
        exit 1
    fi
    cd -
fi

print_blue "Calculating Shuffling Number for model_$MODEL_IDX"

mkdir -p tmp

LINALG_MODEL="${RAROG_ROOT}/tmp/model_${MODEL_IDX}_linalg.mlir"

# Analyze with our tool
$RAROG_OPT_PATH \
    --shuffling-number-pass="verbose=${VERBOSE}" \
    $LINALG_MODEL \
    -o /dev/null # Analysis: output is unchanged
