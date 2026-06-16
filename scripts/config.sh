#!/bin/bash

print_blue() {
    $VERBOSE && echo -e "\e[0;34m * ${1}\e[0m"
}
print_red() {
    $VERBOSE && echo -e "\e[0;31m ! ${1}\e[0m"
}

if [ -f "${RAROG_ROOT}/.env" ]
then
    source "${RAROG_ROOT}/.env"
fi

# Application paths
RAROG_OPT_PATH="${RAROG_OPT_PATH:-${RAROG_ROOT}/build/bin/rarog-opt}"
PYTHON_VENV_PATH="${PYTHON_VENV_PATH:-${RAROG_ROOT}/venv/bin/activate}"
MLIR_INSTALL_DIR="${MLIR_INSTALL_DIR:-/usr/lib/llvm}"
CLANG="${CLANG:-${MLIR_INSTALL_DIR}/bin/clang}"
MLIR_RUNNER="${MLIR_RUNNER:-${MLIR_INSTALL_DIR}/bin/mlir-runner}"
MLIR_TRANSLATE="${MLIR_TRANSLATE:-${MLIR_INSTALL_DIR}/bin/mlir-translate}"

# Libs path
MLIR_LIBS="${MLIR_LIBS:-${MLIR_INSTALL_DIR}/lib}"
MLIR_UTILS="${MLIR_LIBS}/libmlir_runner_utils.so"
MLIR_C_UTILS="${MLIR_LIBS}/libmlir_c_runner_utils.so"
RAROG_LIBS="${RAROG_ROOT}/utils"
INSTRUMENTED_MALLOC="${RAROG_LIBS}/libinstrumented_malloc.so"
STATIC_MALLOC="${RAROG_LIBS}/libstatic_malloc.so"

# Model name
MODEL_PATH="${MODEL_PATH:-$RAROG_ROOT/onnx_models}"
MODEL_NAME="${MODEL_NAME:-model_1}"

# Model intermediate files
ONNX_MODEL="${MODEL_PATH}/${MODEL_NAME}.onnx"
MLIR_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}.mlir"

LINALG_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_linalg.mlir"

DYNAMIC_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic.mlir"
DYNAMIC_LL_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic.ll"

INSTRUMENTED_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.mlir"

STATIC_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_static.mlir"
STATIC_LL_FILE="${RAROG_ROOT}/tmp/${MODEL_NAME}_static.ll"

# Model binaries
DYNAMIC_BINARY="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic"

STATIC_BINARY="${RAROG_ROOT}/tmp/${MODEL_NAME}_static"

# Model log files
DYNAMIC_LOWERING_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic_lowering.log"
DYNAMIC_TRANSLATION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic_translation.log"
DYNAMIC_COMPILATION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic_compilation.log"
DYNAMIC_EXECUTION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic_execution.log"
DYNAMIC_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_dynamic.log"

STATIC_LOWERING_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_lowering.log"
STATIC_TRANSLATION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_translation.log"
STATIC_COMPILATION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_compilation.log"
STATIC_EXECUTION_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_static_execution.log"
STATIC_LOG="${RAROG_ROOT}/tmp/${MODEL_NAME}_static.log"

# Model outputs
INSTRUMENTED_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_instrumented.out"

DYNAMIC_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_dynamic.txt"

STATIC_OUTPUT="${RAROG_ROOT}/tmp/${MODEL_NAME}_output_static.txt"

# Compile rarog-opt
if ! [ -f $RAROG_OPT_PATH ]
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