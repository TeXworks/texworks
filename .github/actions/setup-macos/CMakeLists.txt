cmake_minimum_required(VERSION 3.6)
# 3.6 is required for GIT_SHALLOW

project(TeXworksDependencyInstall)

find_package(Qt5 REQUIRED COMPONENTS Widgets)

# Adapted from https://github.com/Homebrew/homebrew-core/blob/e2c833d326c45d9aaf4e26af6dd8b2f31564dc04/Formula/fontconfig.rb
file(GLOB _font_dirs /System/Library/Assets*/com_apple_MobileAsset_Font*)
list(INSERT _font_dirs 0 /System/Library/Fonts /Library/Fonts ~/Library/Fonts)
string(REPLACE ";" "," _font_dirs "${_font_dirs}")

set(CONFIGURE_ENV CFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} CXXFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
set(CONFIGURE_ARGS --prefix=${CMAKE_INSTALL_PREFIX})

set(CMAKE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_PREFIX_PATH="${CMAKE_INSTALL_PREFIX};${CMAKE_PREFIX_PATH}" -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE -DCMAKE_MACOSX_RPATH=TRUE -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})

include(ExternalProject)

macro(ADD_GH_GROUP target)
	ExternalProject_Add_Step(${target} pre COMMENT "${target} start" COMMAND ${CMAKE_COMMAND} -E echo "::group::${target}" DEPENDERS mkdir ALWAYS TRUE)
	ExternalProject_Add_Step(${target} post COMMENT "${target} end" COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::" DEPENDEES install ALWAYS TRUE)
endmacro(ADD_GH_GROUP)


ExternalProject_Add(libpng
	GIT_REPOSITORY git://git.code.sf.net/p/libpng/code
	GIT_TAG v1.6.37
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(libpng)

# TODO: harfbuzz?
ExternalProject_Add(freetype
	GIT_REPOSITORY https://gitlab.freedesktop.org/freetype/freetype.git
	GIT_TAG VER-2-10-4
	GIT_SHALLOW TRUE
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_SHARED_LIBS=TRUE
	PATCH_COMMAND patch --forward -p1 < ${CMAKE_CURRENT_LIST_DIR}/freetype-bz2.patch
	DEPENDS libpng
)
ADD_GH_GROUP(freetype)

# Modeled after https://github.com/Homebrew/homebrew-core/blob/72ca636/Formula/gettext.rb
ExternalProject_Add(gettext
	URL https://ftp.gnu.org/gnu/gettext/gettext-0.21.tar.xz
	URL_HASH SHA256=d20fcbb537e02dcf1383197ba05bd0734ef7bf5db06bdb241eb69b7d16b73192
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS} --disable-dependency-tracking --disable-silent-rules --disable-debug --with-included-glib --with-included-libcroco --with-included-libunistring --with-included-libxml --with-included-gettext --disable-java --disable-csharp --without-git --without-cvs --without-xz
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(gettext)

ExternalProject_Add(hunspell
	GIT_REPOSITORY https://github.com/hunspell/hunspell.git
	GIT_TAG v1.7.0
	GIT_SHALLOW TRUE
	CONFIGURE_COMMAND autoreconf -vfi && ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
	DEPENDS gettext
)
ADD_GH_GROUP(hunspell)

ExternalProject_Add(fontconfig
	URL https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.13.93.tar.xz
	URL_HASH SHA256=ea968631eadc5739bc7c8856cef5c77da812d1f67b763f5e51b57b8026c1a0a0
	CONFIGURE_COMMAND ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS} --with-add-fonts=${_font_dirs} --disable-docs
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install RUN_FC_CACHE_TEST=false
	DEPENDS freetype gettext
)
ADD_GH_GROUP(fontconfig)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v9d.tar.gz
	URL_HASH SHA256=6c434a3be59f8f62425b2e3c077e785c9ce30ee5874ea1c270e843f273ba71ee
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(libjpeg)

# jbig? Lzma?
ExternalProject_Add(libtiff
	GIT_REPOSITORY https://gitlab.com/libtiff/libtiff.git
	GIT_TAG v4.3.0
	GIT_SHALLOW TRUE
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libjpeg
)
ADD_GH_GROUP(libtiff)

ExternalProject_Add(libopenjpeg
	GIT_REPOSITORY https://github.com/uclouvain/openjpeg.git
	GIT_TAG v2.4.0
	GIT_SHALLOW TRUE
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libtiff
)
ADD_GH_GROUP(libopenjpeg)

ExternalProject_Add(lcms2
	GIT_REPOSITORY https://github.com/mm2/Little-CMS.git
	GIT_TAG lcms2.12
	GIT_SHALLOW TRUE
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS} --with-tiff="${CMAKE_INSTALL_PREFIX}" --with-jpeg="${CMAKE_INSTALL_PREFIX}"
	BUILD_IN_SOURCE TRUE
	DEPENDS libtiff libjpeg
)
ADD_GH_GROUP(lcms2)

# cairo?
ExternalProject_Add(poppler
	GIT_REPOSITORY https://anongit.freedesktop.org/git/poppler/poppler.git
	GIT_TAG poppler-21.04.0
	GIT_SHALLOW TRUE
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_CPP=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE
	DEPENDS fontconfig freetype lcms2 libjpeg libopenjpeg libpng libtiff
)
ADD_GH_GROUP(poppler)

ExternalProject_Add(poppler-data
	URL https://poppler.freedesktop.org/poppler-data-0.4.10.tar.gz
	URL_HASH SHA256=6e2fcef66ec8c44625f94292ccf8af9f1d918b410d5aa69c274ce67387967b30
)
ADD_GH_GROUP(poppler-data)

ExternalProject_Add(lua
	URL http://www.lua.org/ftp/lua-5.4.3.tar.gz
	URL_HASH SHA1=1dda2ef23a9828492b4595c0197766de6e784bc7
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make MYCFLAGS="-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install INSTALL_TOP="${CMAKE_INSTALL_PREFIX}"
)
ADD_GH_GROUP(lua)
