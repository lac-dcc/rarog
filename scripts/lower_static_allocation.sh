#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

ALLOCATION_HEURISTIC="${ALLOCATION_HEURISTIC:-first-fit}"
ILP_FILE="${ILP_FILE:-ilp_allocation_output/model_1.out}"

if ! [ -f $LINALG_MODEL ] || $FRESH
then
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

if ! [ -f $INSTRUMENTED_OUTPUT ] || $FRESH
then
    echo "Instrumentation not found, running instrumentation"
    bash "${RAROG_ROOT}/scripts/run_instrumented.sh" $@
fi

/usr/bin/time --format="time elapsed: %e\n" -o "$STATIC_LOWERING_LOG" $RAROG_OPT_PATH \
    --rarog-lowering-pipeline="$ALLOCATION_HOISTING $DEALLOCATION_HOISTING" \
    --static-allocation="result-file=${INSTRUMENTED_OUTPUT} allocation-heuristic=${ALLOCATION_HEURISTIC}" \
    $LINALG_MODEL -o $STATIC_MODEL > /dev/null
