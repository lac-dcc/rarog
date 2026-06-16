#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $DYNAMIC_MODEL ] || $FRESH
then
    bash "${RAROG_ROOT}/scripts/lower_dynamic_allocation.sh" $@
fi

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "$DYNAMIC_TRANSLATION_LOG" \
    $MLIR_TRANSLATE --mlir-to-llvmir $DYNAMIC_MODEL -o $DYNAMIC_LL_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "$DYNAMIC_COMPILATION_LOG" \
    $CLANG -fuse-ld=lld \
    -Wno-override-module \
    -Wl,-rpath,$MLIR_LIBS \
    -L $MLIR_LIBS \
    -lmlir_runner_utils \
    -lmlir_c_runner_utils \
    -lm -o2 \
    $DYNAMIC_LL_FILE -o $DYNAMIC_BINARY
