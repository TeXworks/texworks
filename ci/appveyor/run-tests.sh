#!/bin/sh

cd "${APPVEYOR_BUILD_FOLDER}/build"

ctest -V
