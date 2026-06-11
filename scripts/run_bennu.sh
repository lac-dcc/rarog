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
for model in $(ls -1 $MODEL_PATH/*.onnx); do
    model_name=$(basename ${model%.onnx})
    # echo "* Create linalg $model_name"

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
    echo "* Instantiate ILP $model_name"
    MODEL_NAME=$model_name ./scripts/instantiator.sh
done

# Run GUROBI
for model_name in "${LINALGS[@]}"; do
    echo "* Invoke GUROBI on $model_name"
    ILP_IN="${RAROG_ROOT}/memory_allocation_input/${model_name}.in"
    ILP_OUT="${RAROG_ROOT}/memory_allocation_output/${model_name}_ilp.out"

    # Already calculated
    if [ -f $ILP_OUT ]; then continue; fi

    python3 ilp/main.py $ILP_IN $ILP_OUT
    echo "* Done with $ILP_OUT"
done

# TODO: Run experiment


