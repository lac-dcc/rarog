#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source ${RAROG_ROOT}/scripts/config.sh
source $PYTHON_VENV_PATH

mkdir -p tmp

# Convert ONNX model to MLIR (torch dialect)
torch-mlir-import-onnx $ONNX_MODEL -o $MLIR_MODEL

# Lower from torch to linalg dialect
torch-mlir-opt \
    --torch-onnx-to-torch-backend-pipeline \
    --torch-backend-to-linalg-on-tensors-backend-pipeline \
    $MLIR_MODEL -o $LINALG_MODEL