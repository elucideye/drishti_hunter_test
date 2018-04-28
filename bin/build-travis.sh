#!/bin/bash

TOOLCHAIN=$1
CONFIG=$2
INSTALL=$3

args=(
    --toolchain ${TOOLCHAIN}
    --config ${CONFIG}
    --ios-multiarch --ios-combined
    --fwd
    GAUZE_ANDROID_USE_EMULATOR=YES
    HUNTER_USE_CACHE_SERVERS=YES
    HUNTER_DISABLE_BUILDS=NO
    HUNTER_CONFIGURATION_TYPES=${CONFIG}
    HUNTER_SUPPRESS_LIST_OF_FILES=ON
    --archive drishti_hunter_test
    --jobs 2
    --test
    ${INSTALL}
)

# Skip --verbose on Android (log is too long)
if [[ ! $name =~ ^android-.* ]]; then 
    args+=(--verbose)
fi

polly.py ${args[@]}
