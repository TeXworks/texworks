cmake_minimum_required(VERSION 3.1)

project(TeXworksDependencyInstall)

if (NOT QT_DEFAULT_MAJOR_VERSION)
	set(QT_DEFAULT_MAJOR_VERSION 5)
endif (NOT QT_DEFAULT_MAJOR_VERSION)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets)

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
	URL "https://download.sourceforge.net/libpng/libpng-1.6.38.tar.xz"
	URL_HASH SHA256=b3683e8b8111ebf6f1ac004ebb6b0c975cd310ec469d98364388e9cedbfa68be
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(libpng)

# TODO: harfbuzz?
ExternalProject_Add(freetype
	URL https://download.savannah.gnu.org/releases/freetype/freetype-2.12.1.tar.xz
	URL_HASH SHA256=4766f20157cc4cf0cd292f80bf917f92d1c439b243ac3018debf6b9140c41a7f
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_SHARED_LIBS=TRUE
	DEPENDS libpng
)
ADD_GH_GROUP(freetype)

# Modeled after https://github.com/Homebrew/homebrew-core/blob/72ca636/Formula/gettext.rb
ExternalProject_Add(gettext
	URL "https://ftp.gnu.org/gnu/gettext/gettext-0.21.1.tar.xz"
	URL_HASH SHA256=50dbc8f39797950aa2c98e939947c527e5ac9ebd2c1b99dd7b06ba33a6767ae6
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS} --disable-dependency-tracking --disable-silent-rules --disable-debug --with-included-glib --with-included-libcroco --with-included-libunistring --with-included-libxml --with-included-gettext --disable-java --disable-csharp --without-git --without-cvs --without-xz
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(gettext)

ExternalProject_Add(hunspell
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.1.tar.gz"
	URL_HASH SHA256=6e3557624c608b3e6525b8bd277706db4f5a857c28fdb3cfa8d0d2b67776da8a
	CONFIGURE_COMMAND autoreconf -vfi && ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
	DEPENDS gettext
)
ADD_GH_GROUP(hunspell)

ExternalProject_Add(fontconfig
	URL "https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.14.1.tar.xz"
	URL_HASH SHA256=298e883f6e11d2c5e6d53c8a8394de58d563902cfab934e6be12fb5a5f361ef0
	CONFIGURE_COMMAND ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS} --with-add-fonts=${_font_dirs} --disable-docs
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install RUN_FC_CACHE_TEST=false
	DEPENDS freetype gettext
)
ADD_GH_GROUP(fontconfig)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v9e.tar.gz
	URL_HASH SHA256=4077d6a6a75aeb01884f708919d25934c93305e49f7e3f36db9129320e6f4f3d
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(libjpeg)

# jbig? Lzma?
ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.4.0.tar.gz"
	URL_HASH SHA256=917223b37538959aca3b790d2d73aa6e626b688e02dcda272aec24c2f498abed
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libjpeg
)
ADD_GH_GROUP(libtiff)

ExternalProject_Add(libopenjpeg
	URL https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.0.tar.gz
	URL_HASH SHA256=0333806d6adecc6f7a91243b2b839ff4d2053823634d4f6ed7a59bc87409122a
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_CODEC=OFF
	DEPENDS libtiff
)
ADD_GH_GROUP(libopenjpeg)

ExternalProject_Add(lcms2
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.14.tar.gz"
	URL_HASH SHA256=05869269f14e589b0b6d05a76f510c68c67fabb304904d16c6bd818abec80a83
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS} --with-tiff="${CMAKE_INSTALL_PREFIX}" --with-jpeg="${CMAKE_INSTALL_PREFIX}"
	BUILD_IN_SOURCE TRUE
	DEPENDS libtiff libjpeg
)
ADD_GH_GROUP(lcms2)

# cairo?
if (DEFINED ENV{MACOSX_DEPLOYMENT_TARGET} AND $ENV{MACOSX_DEPLOYMENT_TARGET} VERSION_LESS "10.15")
ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-21.12.0.tar.xz"
	URL_HASH SHA256=acb840c2c1ec07d07e53c57c4b3a1ff3e3ee2d888d44e1e9f2f01aaf16814de7
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_GLIB=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE
	DEPENDS fontconfig freetype lcms2 libjpeg libopenjpeg libpng libtiff
)
else ()
ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-22.12.0.tar.xz"
	URL_HASH SHA256=d9aa9cacdfbd0f8e98fc2b3bb008e645597ed480685757c3e7bc74b4278d15c0
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_GLIB=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE -DRUN_GPERF_IF_PRESENT=FALSE
	DEPENDS fontconfig freetype lcms2 libjpeg libopenjpeg libpng libtiff
)
endif ()
ADD_GH_GROUP(poppler)

ExternalProject_Add(poppler-data
	URL https://poppler.freedesktop.org/poppler-data-0.4.11.tar.gz
	URL_HASH SHA256=2cec05cd1bb03af98a8b06a1e22f6e6e1a65b1e2f3816cb3069bb0874825f08c
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(poppler-data)

ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.4.4.tar.gz"
	URL_HASH SHA256=164c7849653b80ae67bec4b7473b884bf5cc8d2dca05653475ec2ed27b9ebf61
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make MYCFLAGS="-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install INSTALL_TOP="${CMAKE_INSTALL_PREFIX}"
)
ADD_GH_GROUP(lua)

