#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

echo -e "Processing dynamic allocation $(basename $MODEL_PATH)/$MODEL_NAME"

if ! [ -f $DYNAMIC_BINARY ] || $FRESH
then
    echo "Compiling dynamic binary"
    bash "${RAROG_ROOT}/scripts/compile_dynamic_allocation.sh" $@
    echo "Compilation finished"
fi

DYNAMIC_OUTPUT_TAILED="${DYNAMIC_OUTPUT}.tailed"

# Running dynamic model
echo "Running dynamic model"
/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "$DYNAMIC_EXECUTION_LOG" $DYNAMIC_BINARY > $DYNAMIC_OUTPUT 2> $DYNAMIC_LOG

tail -n1 $DYNAMIC_OUTPUT > $DYNAMIC_OUTPUT_TAILED

echo -e "Finished processing dynamic allocation\e[0m\n\n"



echo -e "Processing static allocation for model $(basename $MODEL_PATH)/$MODEL_NAME"

if ! [ -f $STATIC_BINARY ] || $FRESH
then
    echo "Compiling static binary"
    bash "${RAROG_ROOT}/scripts/compile_static_allocation.sh" $@
    echo "Compilation finished"
fi

STATIC_OUTPUT_TAILED="${STATIC_OUTPUT}.tailed"

# Running static model
echo "Running dynamic model"
/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" \
    -o "$STATIC_EXECUTION_LOG" $STATIC_BINARY > $STATIC_OUTPUT 2> $STATIC_LOG

tail -n1 $STATIC_OUTPUT > $STATIC_OUTPUT_TAILED

echo -e "Finished processing dynamic allocation\e[0m\n"

# Comparing outputs
diff $DYNAMIC_OUTPUT_TAILED $STATIC_OUTPUT_TAILED
if [[ $? = 0 ]]
then
    rm $DYNAMIC_OUTPUT
    rm $DYNAMIC_OUTPUT_TAILED
    rm $STATIC_OUTPUT
    rm $STATIC_OUTPUT_TAILED
else
    echo -e "\e[0;31mERROR:\e[0m Static output differs from dynamic output"
    exit 1
fi