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
    MODEL_NAME=$model_name ./scripts/instantiate.sh
done


