#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. ci/travis-ci/defs.sh

print_headline "Building TeXworks for ${TARGET_OS}/qt${QT}"

echo_and_run "cd \"${BUILDDIR}\""
echo_and_run "make VERBOSE=1"

cd "${TRAVIS_BUILD_DIR}"
