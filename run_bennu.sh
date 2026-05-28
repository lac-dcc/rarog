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
for model_name in $(ls -1 $MODEL_PATH/*.onnx); do
    echo "* Create linalg $(basename ${model_name%.onnx})"
    MODEL_NAME=$(basename ${model_name%.onnx}) ./scripts/create_linalg.sh
done

# # LINALG to ILP
#for model_name in $(ls -1 $MODEL_PATH/*.onnx); do
    # echo "* (TODO) Instantiate ILP $(basename ${model_name%.onnx})"
    # MODEL_NAME=$(basename ${model_name%.onnx}) ./scripts/instantiate.sh
#done

