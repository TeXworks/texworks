#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Building TeXworks for ${TARGET_OS}/qt${QT}"

echo_and_run "cd \"${BUILDDIR}\""
echo_and_run "make VERBOSE=1"

# Run unit tests on supported platforms
if [ "${TARGET_OS}" = "win" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	# For Windows, we are cross-compiling, i.e. we cannot run the produced binaries
	echo "Skipping CTest due to cross-compilation"
elif [ "${TARGET_OS}" = "linux" ]; then
	# Run tests with a (virtual) X server (as the poppler tests require QtGui,
	# e.g. for finding fonts)
	xvfb-run ctest -V
elif [ "${TARGET_OS}" = "osx" ]; then
	ctest -V
fi

cd "${TRAVIS_BUILD_DIR}"
