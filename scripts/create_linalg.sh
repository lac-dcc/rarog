#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

if [ -f ${RAROG_ROOT}/.env ]
then
    source ${RAROG_ROOT}/.env
fi

PYTHON_VENV_PATH="${PYTHON_VENV_PATH:-$RAROG_ROOT/venv/bin/activate}"

source $PYTHON_VENV_PATH

MODEL_PATH="${MODEL_PATH:-$RAROG_ROOT/onnx_models}"
MODEL_NAME="${MODEL_NAME:-model_1}"


# echo "Running model_$MODEL_IDX"

mkdir -p tmp

ONNX_MODEL="${MODEL_PATH}/${MODEL_NAME}.onnx"
MLIR_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}.mlir"
LINALG_MODEL="${RAROG_ROOT}/tmp/${MODEL_NAME}_linalg.mlir"

# Convert ONNX model to MLIR (torch dialect)
torch-mlir-import-onnx $ONNX_MODEL -o $MLIR_MODEL

# Lower from torch to linalg dialect
torch-mlir-opt \
    --torch-onnx-to-torch-backend-pipeline \
    --torch-backend-to-linalg-on-tensors-backend-pipeline \
    $MLIR_MODEL -o $LINALG_MODEL