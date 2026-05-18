RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"
VERBOSE=false
FRESH=true
MODEL_IDX=1

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

print_blue "Invoking Proof of Concept Reorder Transofrm for model_$MODEL_IDX"

mkdir -p tmp

LINALG_MODEL="${RAROG_ROOT}/tmp/model_${MODEL_IDX}_linalg.mlir"
OUTPUT_MODEL="${RAROG_ROOT}/tmp/model_${MODEL_IDX}_poc.mlir"

# Analyize with our tool
$RAROG_OPT_PATH \
    --reorder-proof-of-concept \
    $LINALG_MODEL \
    -o $OUTPUT_MODEL