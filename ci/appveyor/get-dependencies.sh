#!/bin/sh

. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/defs.sh"
. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/package_versions.sh"


print_headline "Installing dependencies"
pacman --noconfirm -S autotools gettext-devel mingw-w64-x86_64-boost mingw-w64-x86_64-freetype mingw-w64-x86_64-openjpeg2 mingw-w64-x86_64-lcms2 mingw-w64-x86_64-libpng mingw-w64-x86_64-libtiff mingw-w64-x86_64-curl mingw-w64-x86_64-lua mingw-w64-x86_64-nss

# /mingw64/ssl/certs/ca-bundle.crt seems to be invalid (empty)
# It is installed by mingw-w64-x86_64-ca-certificates, a dependency of curl
# It is required for verifying https connections
# FIXME: As a temporary workaround, use --insecure with curl

print_headline "Installing poppler"
print_info "Downloading poppler"
mkdir -p /c/projects/poppler
cd /c/projects/poppler
curl -sSL -O --insecure "${poppler_URL}"
# FIXME: Check checksum
print_info "Extracting poppler"
7z x "${poppler_ARCHIVE}" -so | 7z x -si -ttar
cd "${poppler_DIRNAME}"
print_info "Patching poppler"
for PATCH in $(find "${APPVEYOR_BUILD_FOLDER}/.github/actions/setup-windows/mxe/" -iname 'poppler-?-win32.patch'); do
	echo "Applying ${PATCH}"
	patch -p1 < "${PATCH}"
done
print_info "Building poppler"
mkdir build && cd build && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE="Release" -DBUILD_QT6_TESTS=OFF -DENABLE_QT5=OFF -DENABLE_CPP=OFF -DENABLE_UTILS=OFF -DENABLE_UNSTABLE_API_ABI_HEADERS=ON -DENABLE_RELOCATABLE=ON -DENABLE_GPGME=OFF -DCMAKE_INSTALL_PREFIX="/mingw64" .. && make -j && make install


print_headline "Installing hunspell"
print_info "Downloading hunspell"
mkdir -p /c/projects/hunspell
cd /c/projects/hunspell
curl -sSL -O --insecure "${hunspell_URL}"
# FIXME: Check checksum
print_info "Extracting hunspell"
7z x "${hunspell_ARCHIVE}" -so | 7z x -si -ttar
print_info "Building hunspell"
cd "${hunspell_DIRNAME}"
autoreconf --install --force && ./configure && make -j && make install
