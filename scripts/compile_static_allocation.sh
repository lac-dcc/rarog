#!/bin/bash

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"

source "${RAROG_ROOT}/scripts/config_args.sh" $@
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi

if ! [ -f $STATIC_MODEL ] || $FRESH
then
    bash "${RAROG_ROOT}/scripts/lower_static_allocation.sh" $@
fi

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "$STATIC_TRANSLATION_LOG" \
    $MLIR_TRANSLATE --mlir-to-llvmir $STATIC_MODEL -o $STATIC_LL_FILE

/usr/bin/time --format="time elapsed: %e\nmax memory used: %M\n" -o "$STATIC_COMPILATION_LOG" \
    $CLANG -fuse-ld=lld \
    -Wno-override-module \
    -Wl,-rpath,$MLIR_LIBS \
    -Wl,-rpath,$RAROG_LIBS \
    -L $MLIR_LIBS \
    -L $RAROG_LIBS \
    -lmlir_runner_utils \
    -lmlir_c_runner_utils \
    -linstrumented_malloc \
    -lstatic_malloc \
    -lm -o2 \
    $STATIC_LL_FILE -o $STATIC_BINARY
