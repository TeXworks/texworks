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
else
	if [ ${QT} -eq 4 ]; then
		# On Qt4, we need to run tests with an (off-screen) X server
		xvfb-run ctest -V
	else
		ctest -V
	fi
fi

cd "${TRAVIS_BUILD_DIR}"
