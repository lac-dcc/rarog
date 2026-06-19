#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"

# BENNU_MODELS="alexnet googlenet inception_v3 mnasnet1_0 mobilenet_v2 resnet101 resnet152 resnet18 resnet34 resnet50 shufflenet squeezenet"
BENNU_MODELS="alexnet mnasnet1_0 mobilenet_v2 resnet18 resnet34 resnet50 shufflenet squeezenet"

COMPARE_MALLOC_SCRIPT="${RAROG_ROOT}/scripts/compare_malloc.sh"
GET_STATISTICS_SCRIPT="${RAROG_ROOT}/scripts/get_statistics.py"
GEN_CSV_SCRIPT="${RAROG_ROOT}/scripts/gen_csv.py"

# export MODEL_PATH="${RAROG_ROOT}/bennu_models"
export MODEL_PATH="${MODEL_PATH:-$HOME/repos/bennu/models}"
export TMP_RESULTS_PATH="${RAROG_ROOT}/tmp"
export JSON_RESULTS_PATH="${RAROG_ROOT}/Results/Malloc/json/MLIR"
export CSV_RESULT_PATH="${RAROG_ROOT}/Results/Malloc/csv/"

mkdir -p $JSON_RESULTS_PATH $CSV_RESULT_PATH $TMP_RESULTS_PATH

for i in $BENNU_MODELS; do
    export MODEL_NAME=$i
    export ALLOCATION_HEURISTIC=ilp
    echo "Processing model ${i}"
    bash $COMPARE_MALLOC_SCRIPT -df
    if [[ $? = 0 ]]; then
        python3 $GET_STATISTICS_SCRIPT
    else
        echo "Error occurred in comparison"
        exit 1
    fi
done

python3 $GEN_CSV_SCRIPT