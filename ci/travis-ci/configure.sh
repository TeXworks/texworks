#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. ci/travis-ci/defs.sh
. ci/travis-ci/travis-config.sh

print_headline "Configuring for building for ${TARGET_OS}/qt${QT} on ${TRAVIS_OS_NAME}"

BUILDDIR="${TRAVIS_BUILD_DIR}/build-${TRAVIS_OS_NAME}-${TARGET_OS}-qt${QT}"
echo "BUILDDIR=\"${BUILDDIR}\"" >> ci/travis-ci/travis-config.sh

print_info "Making build directory '${BUILDDIR}'"
mkdir "${BUILDDIR}"
cd "${BUILDDIR}"

CMAKE_OPTS="-DTW_BUILD_ID='travis-ci'"

if [ "x${COVERAGE}" != "x" ]; then
	CMAKE_OPTS="${CMAKE_OPTS} -DCMAKE_BUILD_TYPE=\"Debug\" -DWITH_COVERAGE=On"
else
	CMAKE_OPTS="${CMAKE_OPTS} -DCMAKE_BUILD_TYPE=\"Release\""
fi

if [ "${TARGET_OS}" = "linux" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	print_info "Running CMake"
	echo_and_run "cmake .. ${CMAKE_OPTS} -DCMAKE_INSTALL_PREFIX='/usr'"
	if [ -f "CMakeFiles/CMakeError.log" ]; then
		echo "=== CMake Error Log ==="
		cat "CMakeFiles/CMakeError.log"
	fi
elif [ "${TARGET_OS}" = "win" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	print_info "Running CMake"
	echo_and_run "${MXEDIR}/usr/bin/${MXETARGET}-cmake .. \
		${CMAKE_OPTS} \
		-DPLATFORM_DEPENDENCIES='freetype;harfbuzz_too;freetype;bz2;iphlpapi;ssl;crypto;crypt32;ws2_32' \
		-DPoppler_ADDITIONAL_DEPENDENCIES='freetype;harfbuzz;freetype_too;glib-2.0;intl;iconv;ws2_32;winmm;tiff;webp;jpeg;openjp2;png;lcms2;lzma;bz2;pcre16;wtsapi32' \
		-DTEXWORKS_ADDITIONAL_LIBS='wtsapi32;opengl32;imm32;shlwapi;dwmapi;uxtheme' \
		-Dgp_tool='none'"
	if [ -f "CMakeFiles/CMakeError.log" ]; then
		echo "=== CMake Error Log ==="
		cat "CMakeFiles/CMakeError.log"
	fi
elif [ "${TARGET_OS}" = "osx" -a "${TRAVIS_OS_NAME}" = "osx" ]; then
	if [ "${QT}" -eq 5 ]; then
		print_info "Running CMake"
		echo_and_run "cmake .. ${CMAKE_OPTS} -DCMAKE_OSX_SYSROOT=macosx -DCMAKE_PREFIX_PATH=\"/usr/local/opt/qt5\""
		if [ -f "CMakeFiles/CMakeError.log" ]; then
			echo "=== CMake Error Log ==="
			cat "CMakeFiles/CMakeError.log"
		fi
	else
		print_error "Unsupported Qt version '${QT}'"
		exit 1
	fi
else
	print_error "Unsupported host/target combination '${TRAVIS_OS_NAME}/${TARGET_OS}'"
	exit 1
fi

cd "${TRAVIS_BUILD_DIR}"

print_info "Successfully configured build"

