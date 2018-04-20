#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Getting dependencies for building for ${TARGET_OS}/qt${QT} on ${TRAVIS_OS_NAME}"

if [ "${TARGET_OS}" = "linux" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	print_info "Nothing to do"
elif [ "${TARGET_OS}" = "win" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	MXEDIR="/usr/lib/mxe"
	MXETARGET="i686-w64-mingw32.static"

	echo "MXEDIR=\"${MXEDIR}\"" >> travis-ci/defs.sh
	echo "MXETARGET=\"${MXETARGET}\"" >> travis-ci/defs.sh

	print_info "Exporting CC = ${MXETARGET}-gcc"
	CC="${MXETARGET}-gcc"
	print_info "Exporting CXX = ${MXETARGET}-g++"
	CXX="${MXETARGET}-g++"

	JOBS=$(grep '^processor' /proc/cpuinfo | wc -l)

	print_info "Adding pkg.mxe.cc apt repo"
	echo "deb http://pkg.mxe.cc/repos/apt/debian wheezy main" | sudo tee /etc/apt/sources.list.d/mxeapt.list > /dev/null
	sudo apt-key adv --keyserver keyserver.ubuntu.com --recv-keys D43A795B73B16ABE9643FE1AFD8FFF16DB45C6AB
	print_info "Updating apt cache"
	echo_and_run "sudo apt-get -qq update"

	if [ "${QT}" -eq 4 ]; then
		print_info "Installing packages: curl freetype gcc jpeg lcms libpng lua pkgconf qt tiff"
		sudo apt-get install -y mxe-i686-w64-mingw32.static-curl mxe-i686-w64-mingw32.static-freetype mxe-i686-w64-mingw32.static-gcc mxe-i686-w64-mingw32.static-jpeg mxe-i686-w64-mingw32.static-lcms mxe-i686-w64-mingw32.static-libpng mxe-i686-w64-mingw32.static-lua mxe-i686-w64-mingw32.static-pkgconf mxe-i686-w64-mingw32.static-qt mxe-i686-w64-mingw32.static-tiff

		print_info "Make MXE writable"
		sudo chmod -R a+w "${MXEDIR}"

		print_info "Fixing libharfbuzz.la"
		echo_and_run "sed -ie 's#libfreetype.la#libfreetype.la -lharfbuzz_too#g' \"${MXEDIR}/usr/i686-w64-mingw32.static/lib/libharfbuzz.la\""

		cd travis-ci/mxe
		print_info "Building hunspell (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" MXE_CONFIGURE_OPTS="--host='${MXETARGET}' --build='`${MXEDIR}/ext/config.guess`' --prefix='${MXEDIR}/usr/${MXETARGET}' --enable-static --disable-shared" TEST_FILE="hunspell-test.cpp" make -f build-hunspell-mxe.mk

		print_info "Building poppler (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" MXE_CONFIGURE_OPTS="--host='${MXETARGET}' --build='`${MXEDIR}/ext/config.guess`' --prefix='${MXEDIR}/usr/${MXETARGET}' --enable-static --disable-shared ac_cv_prog_HAVE_DOXYGEN='false' --disable-doxygen --enable-poppler-qt4 --disable-poppler-qt5" TEST_FILE="poppler-test.cxx" make -f build-poppler-mxe.mk
	elif [ "${QT}" -eq 5 ]; then
		print_info "Installing packages: curl freetype gcc jpeg lcms libpng lua pkgconf qtbase qtscript qttools tiff"
		sudo apt-get install -y mxe-i686-w64-mingw32.static-curl mxe-i686-w64-mingw32.static-freetype mxe-i686-w64-mingw32.static-gcc mxe-i686-w64-mingw32.static-jpeg mxe-i686-w64-mingw32.static-lcms mxe-i686-w64-mingw32.static-libpng mxe-i686-w64-mingw32.static-lua mxe-i686-w64-mingw32.static-pkgconf mxe-i686-w64-mingw32.static-qtbase mxe-i686-w64-mingw32.static-qtscript mxe-i686-w64-mingw32.static-qttools mxe-i686-w64-mingw32.static-tiff

		print_info "Make MXE writable"
		sudo chmod -R a+w "${MXEDIR}"

		print_info "Fixing libharfbuzz.la"
		echo_and_run "sed -ie 's#libfreetype.la#libfreetype.la -lharfbuzz_too#g' \"${MXEDIR}/usr/i686-w64-mingw32.static/lib/libharfbuzz.la\""

		cd travis-ci/mxe

		print_info "Building hunspell (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt5/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" MXE_CONFIGURE_OPTS="--host='${MXETARGET}' --build='`${MXEDIR}/ext/config.guess`' --prefix='${MXEDIR}/usr/${MXETARGET}' --enable-static --disable-shared" TEST_FILE="hunspell-test.cpp" make -f build-hunspell-mxe.mk

		print_info "Building poppler (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt5/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" MXE_CONFIGURE_OPTS="--host='${MXETARGET}' --build='`${MXEDIR}/ext/config.guess`' --prefix='${MXEDIR}/usr/${MXETARGET}' --enable-static --disable-shared ac_cv_prog_HAVE_DOXYGEN='false' --disable-doxygen --enable-poppler-qt5 --disable-poppler-qt4" TEST_FILE="poppler-test.cxx" make -f build-poppler-mxe.mk
	else
		print_error "Unsupported Qt version '${QT}'"
		exit 1
	fi

elif [ "${TARGET_OS}" = "osx" -a "${TRAVIS_OS_NAME}" = "osx" ]; then
	print_info "Updating homebrew"
	brew update > brew_update.log || { print_error "Updating homebrew failed"; cat brew_update.log; exit 1; }
	if [ $QT -eq 5 ]; then
		print_info "Brewing packages: qt5 poppler hunspell lua"
		# Travis-CI comes with python preinstalled; poppler depends on
		# gobject-introspection, which depends on python@2, which
		# conflicts with the preinstalled version; so we unlink the
		# pre-installed version first
		brew unlink python
		brew install qt5
		brew install "${TRAVIS_BUILD_DIR}/CMake/packaging/mac/poppler.rb" --with-qt --enable-xpdf-headers
	else
		print_error "Unsupported Qt version '${QT}'"
		exit 1
	fi
	brew install hunspell
	brew install lua;
else
	print_error "Unsupported host/target combination '${TRAVIS_OS_NAME}/${TARGET_OS}'"
	exit 1
fi

cd "${TRAVIS_BUILD_DIR}"

print_info "Successfully set up dependencies"
