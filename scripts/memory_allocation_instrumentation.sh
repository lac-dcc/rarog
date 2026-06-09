#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

PYTHON_VENV_PATH="${PYTHON_VENV_PATH:-$RAROG_ROOT/venv/bin/activate}"

source $PYTHON_VENV_PATH


RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"

MLIR_RUNNER=${MLIR_RUNNER:-mlir-runner}
MLIR_UTILS=${MLIR_UTILS:-/usr/lib/llvm/lib/libmlir_runner_utils.so}
MLIR_C_UTILS=${MLIR_C_UTILS:-/usr/lib/llvm/lib/libmlir_c_runner_utils.so}
INSTRUMENTED_MALLOC="${RAROG_ROOT}/utils/libinstrumented_malloc.so"

if [ -z $MODEL_IDX ]
then
    MODEL_IDX=1
fi

ST="${ST:-1}"
ED="${ED:-423625}"

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

get_next_unprocessed() {
    local start=$1
    local end=$2
    local output_dir="${RAROG_ROOT}/memory_allocation_output"
    
    # Get all processed indices in one go
    local processed=$(ls "$output_dir" 2>/dev/null | \
        grep -o 'model_[0-9]*\.out' | \
        sed 's/model_\([0-9]*\)\.out/\1/' | \
        sort -n)
    
    # Find the first gap
    local expected=$start
    for idx in $processed; do
        if [ "$idx" -gt "$expected" ]; then
            echo "$expected"
            return
        fi
        expected=$((idx + 1))
        if [ "$expected" -gt "$end" ]; then
            echo ""  # All done
            return
        fi
    done
    
    # If we processed all consecutive from start, next is expected
    if [ "$expected" -le "$end" ]; then
        echo "$expected"
    else
        echo "$end"  # All done
    fi
}

process_model() {
    local MODEL_IDX=$1
    local OUTPUT_FILE="${RAROG_ROOT}/memory_allocation_output/model_${MODEL_IDX}.out"

    if [ -f $OUTPUT_FILE ]
    then
        return 0
    fi

    local TMP_DIR="tmp_${MODEL_IDX}"
    local ONNX_MODEL="${RAROG_ROOT}/onnx_models/model_${MODEL_IDX}.onnx"
    local MLIR_MODEL="${RAROG_ROOT}/${TMP_DIR}/model_${MODEL_IDX}.mlir"
    local LINALG_MODEL="${RAROG_ROOT}/${TMP_DIR}/model_${MODEL_IDX}_linalg.mlir"
    local INSTRUMENTED_MODEL="${RAROG_ROOT}/${TMP_DIR}/model_${MODEL_IDX}_instrumented.mlir"

    mkdir -p "${RAROG_ROOT}/${TMP_DIR}"

    # Convert ONNX model to MLIR (torch dialect)
    torch-mlir-import-onnx "$ONNX_MODEL" -o "$MLIR_MODEL"

    # Lower from torch to linalg dialect
    torch-mlir-opt \
        --torch-onnx-to-torch-backend-pipeline \
        --torch-backend-to-linalg-on-tensors-backend-pipeline \
        "$MLIR_MODEL" -o "$LINALG_MODEL"

    "$RAROG_OPT_PATH" \
        --rarog-lowering-pipeline \
        --instrument-malloc \
        "$LINALG_MODEL" -o "$INSTRUMENTED_MODEL"

    "$MLIR_RUNNER" \
        "$INSTRUMENTED_MODEL" \
        --entry-point-result=void \
        --shared-libs="$INSTRUMENTED_MALLOC" \
        --shared-libs="$MLIR_UTILS" \
        --shared-libs="$MLIR_C_UTILS" > /dev/null 2> "$OUTPUT_FILE"

    rm -rf "${RAROG_ROOT}/${TMP_DIR}"
}


export -f process_model
export RAROG_ROOT
export RAROG_OPT_PATH
export MLIR_RUNNER
export MLIR_UTILS
export MLIR_C_UTILS
export INSTRUMENTED_MALLOC

if command -v parallel &> /dev/null
then
    seq $(get_next_unprocessed $ST $ED) $ED | parallel --progress -j 4 process_model {}
else
    for MODEL_IDX in $(seq $ST $ED)
    do
        echo -ne "Running model_${MODEL_IDX}\r"
        process_model "$MODEL_IDX"
    done
fi
