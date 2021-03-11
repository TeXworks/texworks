#!/bin/sh

. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/defs.sh"
. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/package_versions.sh"

print_headline "Packaging TeXworks"

# Gather information

# GNU extensions for sed are not supported; on Linux, --posix mimicks this behaviour
TW_VERSION=$(sed -ne 's,^#define TEXWORKS_VERSION[[:space:]]"\([0-9.]\{3\,\}\)"$,\1,p' "${APPVEYOR_BUILD_FOLDER}/src/TWVersion.h")
echo "TW_VERSION = ${TW_VERSION}"

#GIT_HASH=$(git --git-dir=".git" show --no-patch --pretty="%h")
GIT_HASH="${APPVEYOR_REPO_COMMIT:0:7}"
echo "GIT_HASH = ${GIT_HASH}"

DATE_HASH=$(date -u +"%Y%m%d%H%M")
echo "DATE_HASH = ${DATE_HASH}"

VERSION_NAME="${TW_VERSION}-${DATE_HASH}-git_${GIT_HASH}"
echo "VERSION_NAME = ${VERSION_NAME}"

# Make Install

cd "${APPVEYOR_BUILD_FOLDER}/build"

make VERBOSE=1 install

strip -s "${APPVEYOR_BUILD_FOLDER}/artifact/TeXworks.exe"

# Copy Qt dlls
windeployqt --release "${APPVEYOR_BUILD_FOLDER}/artifact/TeXworks.exe"

print_info "Resolving DLL Dependencies"

python "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/resolve-dlls.py" "${APPVEYOR_BUILD_FOLDER}/artifact/TeXworks.exe"
python "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/resolve-dlls.py" "${APPVEYOR_BUILD_FOLDER}/artifact/libTWLuaPlugin.dll"

# Package archive
cd "${APPVEYOR_BUILD_FOLDER}/artifact"
ARCHIVE="TeXworks-win-${VERSION_NAME}.zip"
7z a "$ARCHIVE" -- *

appveyor PushArtifact "${ARCHIVE}"
