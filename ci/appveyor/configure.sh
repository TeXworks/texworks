#!/bin/sh

echo "Configuring TeXworks"

mkdir -p "${APPVEYOR_BUILD_FOLDER}/build"
cd "${APPVEYOR_BUILD_FOLDER}/build"

cmake -G"MSYS Makefiles" -DTW_BUILD_ID='appveyor' -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX="${APPVEYOR_BUILD_FOLDER}/artifact" -DTEXWORKS_ADDITIONAL_LIBS="shlwapi" ..
