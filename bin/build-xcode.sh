#!/bin/bash

TOOLCHAIN=xcode
CONFIG=Release

COMMAND=(
    "--verbose "
    "--fwd "
    "HUNTER_CONFIGURATION_TYPES=${CONFIG} "    
    "DRISHTI_BUILD_BENCHMARKS=OFF "
    "DRISHTI_BUILD_TESTS=OFF "
    "DRISHTI_BUILD_INTEGRATION_TESTS=OFF "
    "DRISHTI_BUILD_SHARED_SDK=OFF "    
    "--config ${CONFIG} "
    "--jobs 8 "
    "--install "
)

polly.py --toolchain ${TOOLCHAIN} ${COMMAND[@]} --verbose #  --reconfig
