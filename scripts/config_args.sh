#!/bin/bash

usage() {
    echo "Usage: $0 [-a] [-d] [-f] [-h]"
    echo ""
    echo "Options:"
    echo "  -a              Enable allocation hoisting"
    echo "  -d              Enable deallocation hoisting"
    echo "  -f              Force recompile of models"
    echo "  -h              Show this help message"
    exit 1
}

RAROG_ROOT="$(cd "$(dirname ${BASH_SOURCE[0]})/.." && pwd)"
RAROG_OPT_PATH="${RAROG_ROOT}/build/bin/rarog-opt"
ALLOCATION_HOISTING=""
DEALLOCATION_HOISTING=""
FRESH=false

while getopts "adfh" OPTION; do
  case $OPTION in
    a)  ALLOCATION_HOISTING="enable-reorder-mallocs" ;;
    d)  DEALLOCATION_HOISTING="enable-reorder-frees" ;;
    f)  FRESH=true ;;
    h)  usage ;;
 esac
done

source "${RAROG_ROOT}/scripts/config.sh"
RETURN_CODE=$?
if [[ $RETURN_CODE != 0 ]]
then
    exit $RETURN_CODE
fi