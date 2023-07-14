#!/usr/bin/env bash

SCRIPT=$(realpath "$0")
SCRIPT_PATH=$(dirname "$SCRIPT")
PROJECT_DIR="${PROJECT_DIR:-$SCRIPT_PATH/..}"
BUILD_TYPE="${BUILD_TYPE:-Debug}"
BUILD_DIR="${BUILD_DIR:-$SCRIPT_PATH/../build/$BUILD_TYPE}"
GAME="${GAME:-ut99}"
URL="${URL:-DM-Tutorial.unr}"

"${BUILD_DIR}/SurrealEngine" "${PROJECT_DIR}/games/${GAME}" "--url=${URL}" |
  ts '[%d-%m-%Y %H:%M:%.S]' | tee "/tmp/${GAME}.log"
