#!/bin/bash

# Get bennu models (3GB)
: '
wget https://github.com/git-lfs/git-lfs/releases/download/v3.7.1/git-lfs-linux-amd64-v3.7.1.tar.gz
tar -xzf git-lfs-linux-amd64-v3.7.1.tar.gz
cd git-lfs-3.7.1/
sudo ./install.sh 
git lfs install
cd ..
git clone https://github.com/lac-dcc/bennu
cd bennu
git lfs track "*.onnx"
git lfs pull
'
RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
export MODEL_PATH="${MODEL_PATH:-$HOME/repos/bennu/models}"

# ONNX to LINALG
declare -a LINALGS

# ? Some networks didn't lower to linalg...
#for model in $(ls -1 $MODEL_PATH/*.onnx); do
    # model_name=$(basename ${model%.onnx})
    # echo "* Create linalg $model_name"

MODELS=(alexnet
    # densenet121 densenet161 densenet169 densenet201
    googlenet inception_v3 mnasnet1_0 mobilenet_v2
    resnet101 resnet152 resnet18 resnet34
    resnet50 shufflenet squeezenet vgg11
    # vgg13
    vgg16 vgg19)

for model_name in "${MODELS[@]}"; do
    LINALG_MODEL="${RAROG_ROOT}/tmp/${model_name}_linalg.mlir"
    if [ -f $LINALG_MODEL ]; then
        LINALGS+=($model_name)
        continue
    fi

    # Run the script
    MODEL_NAME=$model_name ./scripts/create_linalg.sh

    # Success: add to list
    if [[ $? -eq 0 ]]; then
        LINALGS+=($model_name)
    else
        echo "Model $model_name failed!"
    fi
done

echo "${#LINALGS[@]} models were successful:"

# LINALG to ILP
for model_name in "${LINALGS[@]}"; do
    OUTPUT_FILE="${RAROG_ROOT}/memory_allocation_input/${model_name}.in"
    if [ -f $OUTPUT_FILE ]; then continue; fi

    echo "* Instantiate ILP $model_name"
    MODEL_NAME=$model_name ./scripts/instantiator.sh
done

source venv/bin/activate

# Run GUROBI
for model_name in "${LINALGS[@]}"; do
    ILP_IN="${RAROG_ROOT}/memory_allocation_input/${model_name}.in"
    ILP_OUT="${RAROG_ROOT}/memory_allocation_output/${model_name}_ilp.out"

    # Already calculated
    if [ -f $ILP_OUT ]; then continue; fi

    echo "* Invoke GUROBI on $model_name"

    python3 ilp/gurobi.py $ILP_IN $ILP_OUT
    echo "* Done with $ILP_OUT"
done

# Run experiment
for model_name in "${LINALGS[@]}"; do
    ILP_FILE="${RAROG_ROOT}/memory_allocation_output/${model_name}_ilp.out"
    STATIC_ALLOCATION_MODEL="${RAROG_ROOT}/tmp/${model_name}_static.mlir"

    # Already lowered
    if [ -f $STATIC_ALLOCATION_MODEL ]; then continue; fi

    echo "* Lower Static Allocation on $model_name"

    MODEL_NAME="${model_name}" ILP_FILE="${ILP_FILE}" \
        ./scripts/lower_static_allocation.sh

    echo "* Lowered with $STATIC_ALLOCATION_MODEL"
done

# Compile the model to binary
for model_name in "${LINALGS[@]}"; do
    STATIC_BINARY="${RAROG_ROOT}/tmp/${model_name}_static"
    if [ -f $STATIC_BINARY ]; then continue; fi

    echo "* Compiling $model_name"
    MODEL_NAME="${model_name}" \
        ./scripts/compile_static_allocation.sh
done


