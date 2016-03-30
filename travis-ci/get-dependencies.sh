#!/usr/bin/env sh

# Exit on errors
set -e

cd "${TRAVIS_BUILD_DIR}"

. travis-ci/defs.sh

print_headline "Getting dependencies for building for ${TARGET_OS}/qt${QT} on ${TRAVIS_OS_NAME}"

if [ "${TARGET_OS}" = "linux" -a "${TRAVIS_OS_NAME}" = "linux" ]; then
	print_info "Updating apt cache"
	sudo apt-get -qq update
	if [ $QT -eq 4 ]; then
		print_info "Installing packages: ubuntu-dev-tools dput debhelper libqt4-dev zlib1g-dev libhunspell-dev libpoppler-qt4-dev liblua5.2-dev"
		sudo apt-get install -y ubuntu-dev-tools dput debhelper libqt4-dev zlib1g-dev libhunspell-dev libpoppler-qt4-dev liblua5.2-dev
	elif [ $QT -eq 5 ]; then
		print_info "Installing packages: ubuntu-dev-tools dput debhelper qtbase5-dev qtscript5-dev qttools5-dev zlib1g-dev libhunspell-dev libpoppler-qt5-dev libpoppler-private-dev liblua5.2-dev"
		sudo apt-get install -y ubuntu-dev-tools dput debhelper qtbase5-dev qtscript5-dev qttools5-dev zlib1g-dev libhunspell-dev libpoppler-qt5-dev libpoppler-private-dev liblua5.2-dev
	else
		print_error "Unsupported Qt version '${QT}'"
		exit 1
	fi
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
		print_info "Installing packages: curl freetype gcc hunspell jpeg lcms1 libpng lua pkg-config qt tiff"
		sudo apt-get install -y mxe-i686-w64-mingw32.static-curl mxe-i686-w64-mingw32.static-freetype mxe-i686-w64-mingw32.static-gcc mxe-i686-w64-mingw32.static-hunspell mxe-i686-w64-mingw32.static-jpeg mxe-i686-w64-mingw32.static-lcms1 mxe-i686-w64-mingw32.static-libpng mxe-i686-w64-mingw32.static-lua mxe-i686-w64-mingw32.static-pkgconf mxe-i686-w64-mingw32.static-qt mxe-i686-w64-mingw32.static-tiff

		print_info "Make MXE writable"
		sudo chmod -R a+w "${MXEDIR}"

		cd travis-ci/mxe
		print_info "Building poppler (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" make -f build-poppler-mxe.mk
	elif [ "${QT}" -eq 5 ]; then
		print_info "Installing packages: curl freetype gcc hunspell jpeg lcms1 libpng lua pkg-config qtbase qtscript qttools tiff"
		sudo apt-get install -y mxe-i686-w64-mingw32.static-curl mxe-i686-w64-mingw32.static-freetype mxe-i686-w64-mingw32.static-gcc mxe-i686-w64-mingw32.static-hunspell mxe-i686-w64-mingw32.static-jpeg mxe-i686-w64-mingw32.static-lcms1 mxe-i686-w64-mingw32.static-libpng mxe-i686-w64-mingw32.static-lua mxe-i686-w64-mingw32.static-pkgconf mxe-i686-w64-mingw32.static-qtbase mxe-i686-w64-mingw32.static-qtscript mxe-i686-w64-mingw32.static-qttools mxe-i686-w64-mingw32.static-tiff

		print_info "Make MXE writable"
		sudo chmod -R a+w "${MXEDIR}"

		cd travis-ci/mxe
		print_info "Patching MXE Qt5 (see https://github.com/mxe/mxe/issues/1185)"
		patch -f -d "${MXEDIR}/usr/${MXETARGET}" -p1 < qt5-QUiLoader-fix.patch || ( echo "Patching failed; maybe the patch is already applied?"; cat "${MXEDIR}/usr/${MXETARGET}/qt5/lib/cmake/Qt5UiPlugin/Qt5UiPluginConfig.cmake" )

		print_info "Building poppler (using ${JOBS} jobs)"
		env PATH="${MXEDIR}/usr/bin:${MXEDIR}/usr/${MXETARGET}/qt5/bin:$PATH" PREFIX="${MXEDIR}/usr" TARGET="${MXETARGET}" JOBS="$JOBS" make -f build-poppler-mxe.mk
	else
		print_error "Unsupported Qt version '${QT}'"
		exit 1
	fi

elif [ "${TARGET_OS}" = "osx" -a "${TRAVIS_OS_NAME}" = "osx" ]; then
	print_info "Updating homebrew"
	brew update > brew_update.log || { print_error "Updating homebrew failed"; cat brew_update.log; exit 1; }
	if [ $QT -eq 4 ]; then
		print_info "Brewing packages: qt4 poppler hunspell lua"
		brew install qt4
		brew install "${TRAVIS_BUILD_DIR}/CMake/packaging/mac/poppler.rb" --with-qt --enable-xpdf-headers
	elif [ $QT -eq 5 ]; then
		print_info "Brewing packages: qt5 poppler hunspell lua"
		brew install qt5
		brew install "${TRAVIS_BUILD_DIR}/CMake/packaging/mac/poppler.rb" --with-qt5 --enable-xpdf-headers
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
