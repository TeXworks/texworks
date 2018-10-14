#!/usr/bin/env bash

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Testing TeXworks for ${TARGET_OS}/qt${QT}"

echo_and_run "cd \"${BUILDDIR}\""

if [ "x${COVERAGE}" != "x" ]; then
	lcov --zerocounters --directory .
	lcov --capture --initial --directory . --output-file "coverage.base"
fi

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

if [ "x${COVERAGE}" != "x" ]; then
	lcov --capture --directory . --output-file "coverage.info"
	lcov --add-tracefile "coverage.base" --add-tracefile "coverage.info" --output-file "coverage.info"
	lcov --remove "coverage.info" '/usr/*' --output-file "coverage.info"
	lcov --list coverage.info #debug info
	bash <(curl -s https://codecov.io/bash) || echo "Codecov did not collect coverage reports"
fi

cd "${TRAVIS_BUILD_DIR}"
