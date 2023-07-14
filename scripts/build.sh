#!/usr/bin/env bash

SCRIPT=$(realpath "$0")
SCRIPT_PATH=$(dirname "$SCRIPT")
PROJECT_DIR="${PROJECT_DIR:-$SCRIPT_PATH/..}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_PATH/../build/$BUILD_TYPE}"

cmake --no-warn-unused-cli -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=TRUE \
  -DCMAKE_BUILD_TYPE:STRING=Debug -DCMAKE_C_COMPILER:FILEPATH=/usr/bin/clang \
  -DCMAKE_CXX_COMPILER:FILEPATH=/usr/bin/clang++ -S${PROJECT_DIR} \
  -B${BUILD_DIR} \
  -G "Unix Makefiles" &&
  cmake --build ${BUILD_DIR} --config ${BUILD_TYPE} --target all -j 18 --