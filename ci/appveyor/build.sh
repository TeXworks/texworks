#!/bin/sh

cd "${APPVEYOR_BUILD_FOLDER}/build"

make VERBOSE=1
