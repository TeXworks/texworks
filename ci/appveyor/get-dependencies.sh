#!/bin/sh

. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/defs.sh"
. "${APPVEYOR_BUILD_FOLDER}/ci/appveyor/package_versions.sh"


print_headline "Installing dependencies"
pacman --noconfirm -S mingw-w64-x86_64-freetype mingw-w64-x86_64-openjpeg2 mingw-w64-x86_64-lcms2 mingw-w64-x86_64-libpng mingw-w64-x86_64-libtiff mingw-w64-x86_64-curl mingw-w64-x86_64-lua

print_headline "Installing poppler-data"
print_info "Downloading poppler-data"
mkdir -p /c/projects/poppler-data
cd /c/projects/poppler-data
curl -sSL -O "${popplerdata_URL}"
# FIXME: Check checksum
print_info "Extracting poppler-data"
7z x "${popplerdata_ARCHIVE}" -so | 7z x -si -ttar
print_info "Renaming ${popplerdata_DIRNAME} > poppler-data"
mv "${popplerdata_DIRNAME}" poppler-data


print_headline "Installing poppler"
print_info "Downloading poppler"
mkdir -p /c/projects/poppler
cd /c/projects/poppler
curl -sSL -O "${poppler_URL}"
# FIXME: Check checksum
print_info "Extracting poppler"
7z x "${poppler_ARCHIVE}" -so | 7z x -si -ttar
cd "${poppler_DIRNAME}"
print_info "Patching poppler"
for PATCH in $(find "${APPVEYOR_BUILD_FOLDER}/ci/travis-ci/mxe/" -iname 'poppler-*.patch'); do
	echo "Applying ${PATCH}"
	patch -p1 < "${PATCH}"
done
print_info "Building poppler"
mkdir build && cd build && cmake -G"MSYS Makefiles" -DCMAKE_BUILD_TYPE="Release" -DBUILD_QT5_TESTS=OFF -DENABLE_CPP=OFF -DENABLE_UTILS=OFF -DENABLE_UNSTABLE_API_ABI_HEADERS=ON -DCMAKE_INSTALL_PREFIX="/mingw64" .. && make -j && make install


print_headline "Installing hunspell"
print_info "Downloading hunspell"
mkdir -p /c/projects/hunspell
cd /c/projects/hunspell
curl -sSL -O "${hunspell_URL}"
# FIXME: Check checksum
print_info "Extracting hunspell"
7z x "${hunspell_ARCHIVE}" -so | 7z x -si -ttar
print_info "Building hunspell"
cd "${hunspell_DIRNAME}"
autoreconf -i && ./configure && make -j && make install
