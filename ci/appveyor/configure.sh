#!/bin/sh

echo "Configuring TeXworks"

mkdir -p "${APPVEYOR_BUILD_FOLDER}/build"
cd "${APPVEYOR_BUILD_FOLDER}/build"


echo "Copying poppler-data"
# Extract poppler-data (required by some tests)
mkdir -p share
echo "cp -r /c/projects/poppler-data/${popplerdata_DIRNAME} share/poppler"
cp -r /c/projects/poppler-data/poppler-data share/poppler

echo "Copying fonts"
# Copy extra fonts (required by some tests)
cp -r "${APPVEYOR_BUILD_FOLDER}/win32/fonts" share/fonts

cmake -G"MSYS Makefiles" -DTW_BUILD_ID='appveyor' -DCMAKE_BUILD_TYPE='Release' -DCMAKE_INSTALL_PREFIX="${APPVEYOR_BUILD_FOLDER}/artifact" -DTEXWORKS_ADDITIONAL_LIBS="shlwapi" ..
