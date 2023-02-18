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

set(CMAKE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} "-DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}\\;${CMAKE_PREFIX_PATH}" -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE -DCMAKE_MACOSX_RPATH=TRUE -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})

include(ExternalProject)

macro(ADD_GH_GROUP target)
	ExternalProject_Add_Step(${target} pre COMMENT "${target} start" COMMAND ${CMAKE_COMMAND} -E echo "::group::${target}" DEPENDERS mkdir ALWAYS TRUE)
	ExternalProject_Add_Step(${target} post COMMENT "${target} end" COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::" DEPENDEES install ALWAYS TRUE)
endmacro(ADD_GH_GROUP)


ExternalProject_Add(libpng
	URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.39.tar.gz"
	URL_HASH SHA256=a00e9d2f2f664186e4202db9299397f851aea71b36a35e74910b8820e380d441
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(libpng)

# TODO: harfbuzz?
ExternalProject_Add(freetype
	URL https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-0.tar.gz
	URL_HASH SHA256=a683f1091aee95d2deaca9292d976f87415610b8ae1ea186abeebcb08e83ab12
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
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.2.tar.gz"
	URL_HASH SHA256=69fa312d3586c988789266eaf7ffc9861d9f6396c31fc930a014d551b59bbd6e
	CONFIGURE_COMMAND autoreconf -vfi && ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
	DEPENDS gettext
)
ADD_GH_GROUP(hunspell)

ExternalProject_Add(fontconfig
	URL "https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.14.2.tar.xz"
	URL_HASH SHA256=dba695b57bce15023d2ceedef82062c2b925e51f5d4cc4aef736cf13f60a468b
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

# jbig? Lzma? Deflate?
ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.5.0.tar.gz"
	URL_HASH SHA256=c7a1d9296649233979fa3eacffef3fa024d73d05d589cb622727b5b08c423464
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
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS} "--with-tiff=${CMAKE_INSTALL_PREFIX}" "--with-jpeg=${CMAKE_INSTALL_PREFIX}"
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
	URL "https://poppler.freedesktop.org/poppler-23.02.0.tar.xz"
	URL_HASH SHA256=3315dda270fe2b35cf1f41d275948c39652fa863b90de0766f6b293d9a558fc9
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_GLIB=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE -DRUN_GPERF_IF_PRESENT=FALSE
	DEPENDS fontconfig freetype lcms2 libjpeg libopenjpeg libpng libtiff
)
endif ()
ADD_GH_GROUP(poppler)

# Need to install poppler-data (system-wide) so the test-cases succeed
# (NB: poppler-data is downloaded and installed separately during packaging,
# but only in the TeXworks.app directory, which doesn't help the tests)
ExternalProject_Add(poppler-data
	URL https://poppler.freedesktop.org/poppler-data-0.4.12.tar.gz
	URL_HASH SHA256=c835b640a40ce357e1b83666aabd95edffa24ddddd49b8daff63adb851cdab74
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(poppler-data)

ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.4.4.tar.gz"
	URL_HASH SHA256=164c7849653b80ae67bec4b7473b884bf5cc8d2dca05653475ec2ed27b9ebf61
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make "MYCFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install "INSTALL_TOP=${CMAKE_INSTALL_PREFIX}"
)
ADD_GH_GROUP(lua)

