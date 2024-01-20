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
	URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.40.tar.gz"
	URL_HASH SHA256=62d25af25e636454b005c93cae51ddcd5383c40fa14aa3dae8f6576feb5692c2
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(libpng)

# TODO: harfbuzz?
ExternalProject_Add(freetype
	URL "https://github.com/freetype/freetype/archive/refs/tags/VER-2-13-2.tar.gz"
	URL_HASH SHA256=427201f5d5151670d05c1f5b45bef5dda1f2e7dd971ef54f0feaaa7ffd2ab90c
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_SHARED_LIBS=TRUE
	DEPENDS libpng
)
ADD_GH_GROUP(freetype)

# Modeled after https://github.com/Homebrew/homebrew-core/blob/72ca636/Formula/gettext.rb
ExternalProject_Add(gettext
	URL "https://ftp.gnu.org/gnu/gettext/gettext-0.22.4.tar.xz"
	URL_HASH SHA256=29217f1816ee2e777fa9a01f9956a14139c0c23cc1b20368f06b2888e8a34116
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
	URL "https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.15.0.tar.xz"
	URL_HASH SHA256=63a0658d0e06e0fa886106452b58ef04f21f58202ea02a94c39de0d3335d7c0e
	CONFIGURE_COMMAND ${CONFIGURE_ENV} CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS} --with-add-fonts=${_font_dirs} --disable-docs
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install RUN_FC_CACHE_TEST=false
	DEPENDS freetype gettext
)
ADD_GH_GROUP(fontconfig)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v9f.tar.gz
	URL_HASH SHA256=04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b
	CONFIGURE_COMMAND ${CONFIGURE_ENV} ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(libjpeg)

# jbig? Lzma? Deflate?
ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.6.0.tar.gz"
	URL_HASH SHA256=88b3979e6d5c7e32b50d7ec72fb15af724f6ab2cbf7e10880c360a77e4b5d99a
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
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.16.tar.gz"
	URL_HASH SHA256=cb4bf968aa66e3d5d16d1f094a99595966d4330cbd6be46c66129f2747ad2b0d
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
	URL "https://poppler.freedesktop.org/poppler-24.01.0.tar.xz"
	URL_HASH SHA256=c7def693a7a492830f49d497a80cc6b9c85cb57b15e9be2d2d615153b79cae08
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=OFF -DENABLE_CPP=OFF -DENABLE_GLIB=OFF -DENABLE_UTILS=OFF -DENABLE_UNSTABLE_API_ABI_HEADERS=ON -DRUN_GPERF_IF_PRESENT=OFF -DENABLE_QT5=OFF -DBUILD_MANUAL_TESTS=OFF -DBUILD_QT5_TESTS=OFF -DENABLE_NSS3=OFF -DENABLE_GPGME=OFF -DENABLE_LIBCURL=OFF
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
	URL "https://www.lua.org/ftp/lua-5.4.6.tar.gz"
	URL_HASH SHA256=7d5ea1b9cb6aa0b59ca3dde1c6adcb57ef83a1ba8e5432c0ecd06bf439b3ad88
	CONFIGURE_COMMAND ""
	BUILD_COMMAND make "MYCFLAGS=-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET}"
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install "INSTALL_TOP=${CMAKE_INSTALL_PREFIX}"
)
ADD_GH_GROUP(lua)

