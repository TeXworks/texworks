#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Building TeXworks for ${TARGET_OS}/qt${QT}"

echo_and_run "cd \"${BUILDDIR}\""
echo_and_run "make VERBOSE=1"

# Run unit tests (except for Windows builds which we are cross-compiling)
if [ ! ("${TARGET_OS}" = "win" -a "${TRAVIS_OS_NAME}" = "linux") ]; then
	ctest -V
fi

cd "${TRAVIS_BUILD_DIR}"
