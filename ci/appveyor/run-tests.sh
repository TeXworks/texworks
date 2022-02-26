#!/bin/sh

cd "${APPVEYOR_BUILD_FOLDER}/build"

# Copy poppler dlls to here as poppler is by default looking for relevant data
# (e.g., poppler-data, fonts) relative to its location
cp /mingw64/bin/libpoppler*.dll .

ctest -V
