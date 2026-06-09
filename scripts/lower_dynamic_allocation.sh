#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $LINALG_MODEL ] || $FRESH
then
    bash ${RAROG_ROOT}/scripts/create_linalg.sh
fi

# Apply lowering pipeline
/usr/bin/time --format="time elapsed: %e\n" -o "$DYNAMIC_LOWERING_LOG" $RAROG_OPT_PATH \
    --rarog-lowering-pipeline="$ALLOCATION_HOISTING $DEALLOCATION_HOISTING" \
    $LINALG_MODEL -o $DYNAMIC_MODEL