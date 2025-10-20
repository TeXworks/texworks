cmake_minimum_required(VERSION 3.1...4.0)

project(TeXworksDependencyInstall)

if (NOT QT_DEFAULT_MAJOR_VERSION)
	set(QT_DEFAULT_MAJOR_VERSION 5)
endif (NOT QT_DEFAULT_MAJOR_VERSION)

find_package(Qt${QT_DEFAULT_MAJOR_VERSION} REQUIRED COMPONENTS Widgets)

# Adapted from https://github.com/Homebrew/homebrew-core/blob/e2c833d326c45d9aaf4e26af6dd8b2f31564dc04/Formula/fontconfig.rb
file(GLOB _font_dirs /System/Library/Assets*/com_apple_MobileAsset_Font*)
list(INSERT _font_dirs 0 /System/Library/Fonts /Library/Fonts ~/Library/Fonts)
string(REPLACE ";" "," _font_dirs "${_font_dirs}")

set(CONFIGURE_ARGS --prefix=${CMAKE_INSTALL_PREFIX})
set(CONFIGURE_CFLAGS -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
foreach(ARCH IN LISTS CMAKE_OSX_ARCHITECTURES)
	set(ARCH_FLAGS "${ARCH_FLAGS} -arch ${ARCH}")
	if (ARCH STREQUAL "arm64")
		set(CONFIGURE_ARGS ${CONFIGURE_ARGS} --host=aarch64-apple-darwin)
	else ()
		set(CONFIGURE_ARGS ${CONFIGURE_ARGS} --host=${ARCH}-apple-darwin)
	endif()
endforeach()
set(CONFIGURE_ENV "CFLAGS=\"${CONFIGURE_CFLAGS} ${ARCH_FLAGS}\" CXXFLAGS=\"${CONFIGURE_CFLAGS} ${ARCH_FLAGS}\"")

set(CMAKE_ARGS -DCMAKE_OSX_DEPLOYMENT_TARGET=${CMAKE_OSX_DEPLOYMENT_TARGET} -DCMAKE_OSX_ARCHITECTURES=${CMAKE_OSX_ARCHITECTURES} "-DCMAKE_PREFIX_PATH=${CMAKE_INSTALL_PREFIX}\\;${CMAKE_PREFIX_PATH}" -DCMAKE_INSTALL_RPATH_USE_LINK_PATH=TRUE -DCMAKE_MACOSX_RPATH=TRUE -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX})

include(ExternalProject)

macro(ADD_GH_GROUP target)
	if (CMAKE_VERSION VERSION_LESS "3.19")
		ExternalProject_Add_Step(${target} pre COMMENT "${target} start" COMMAND ${CMAKE_COMMAND} -E echo "::group::${target}" DEPENDERS mkdir ALWAYS TRUE)
		ExternalProject_Add_Step(${target} post COMMENT "${target} end" COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::" DEPENDEES install ALWAYS TRUE)
	else()
		ExternalProject_Add_Step(${target} pre COMMENT "${target} start" COMMAND ${CMAKE_COMMAND} -E echo "::group::${target}" DEPENDERS mkdir ALWAYS TRUE INDEPENDENT TRUE>)
		ExternalProject_Add_Step(${target} post COMMENT "${target} end" COMMAND ${CMAKE_COMMAND} -E echo "::endgroup::" DEPENDEES install ALWAYS TRUE)
	endif()
endmacro(ADD_GH_GROUP)


ExternalProject_Add(libpng
	URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.50.tar.gz"
	URL_HASH SHA256=71158e53cfdf2877bc99bcab33641d78df3f48e6e0daad030afe9cb8c031aa46
	CMAKE_ARGS ${CMAKE_ARGS}
)
ADD_GH_GROUP(libpng)

# TODO: harfbuzz?
ExternalProject_Add(freetype
	URL "https://github.com/freetype/freetype/archive/refs/tags/VER-2-14-1.tar.gz"
	URL_HASH SHA256=44bd69d1f0750603410cfd4f26f1c5523c5a3a087a2dfa8639b757dc7c6677be
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_SHARED_LIBS=TRUE
	DEPENDS libpng
)
ADD_GH_GROUP(freetype)

# Modeled after https://github.com/Homebrew/homebrew-core/blob/72ca636/Formula/gettext.rb
ExternalProject_Add(gettext
	URL "https://ftp.gnu.org/gnu/gettext/gettext-0.26.tar.xz"
	URL_HASH SHA256=d1fb86e260cfe7da6031f94d2e44c0da55903dbae0a2fa0fae78c91ae1b56f00
	CONFIGURE_COMMAND autoreconf --force && env -S "${CONFIGURE_ENV} am_cv_func_iconv_works=yes" ./configure ${CONFIGURE_ARGS} --disable-dependency-tracking --disable-silent-rules --disable-debug --with-included-glib --with-included-libcroco --with-included-libunistring --with-included-libxml --with-included-gettext --disable-java --disable-csharp --without-git --without-cvs --without-xz
	BUILD_IN_SOURCE TRUE
)

ADD_GH_GROUP(gettext)

# Need to specify CXX as apparently that is used somewhere down the line for
# linking (!) but without getting passed the necessary flags
ExternalProject_Add(hunspell
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.2.tar.gz"
	URL_HASH SHA256=69fa312d3586c988789266eaf7ffc9861d9f6396c31fc930a014d551b59bbd6e
	CONFIGURE_COMMAND autoreconf -vfi && env -S "CXX=\"g++ ${ARCH_FLAGS}\" ${CONFIGURE_ENV} LDFLAGS=\"-L${CMAKE_INSTALL_PREFIX}/lib ${ARCH_FLAGS}\" CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include" ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
	DEPENDS gettext
)
ADD_GH_GROUP(hunspell)

# Need to specify CXX as apparently that is used somewhere down the line for
# linking (!) but without getting passed the necessary flags
ExternalProject_Add(fontconfig
	URL "https://www.freedesktop.org/software/fontconfig/release/fontconfig-2.16.0.tar.xz"
	URL_HASH SHA256=6a33dc555cc9ba8b10caf7695878ef134eeb36d0af366041f639b1da9b6ed220
	PATCH_COMMAND ./autogen.sh
	CONFIGURE_COMMAND env -S "${CONFIGURE_ENV} CXX=\"g++ ${ARCH_FLAGS}\"" CPPFLAGS=-I${CMAKE_INSTALL_PREFIX}/include LDFLAGS=-L${CMAKE_INSTALL_PREFIX}/lib ./configure ${CONFIGURE_ARGS} --disable-silent-rules --with-add-fonts=${_font_dirs} --disable-docs
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install RUN_FC_CACHE_TEST=false
	DEPENDS freetype gettext
)
ADD_GH_GROUP(fontconfig)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v9f.tar.gz
	URL_HASH SHA256=04705c110cb2469caa79fb71fba3d7bf834914706e9641a4589485c1f832565b
	CONFIGURE_COMMAND env -S "${CONFIGURE_ENV}" ./configure ${CONFIGURE_ARGS}
	BUILD_IN_SOURCE TRUE
)
ADD_GH_GROUP(libjpeg)

# jbig? Lzma? Deflate?
ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.7.1.tar.gz"
	URL_HASH SHA256=f698d94f3103da8ca7438d84e0344e453fe0ba3b7486e04c5bf7a9a3fabe9b69
	CMAKE_ARGS ${CMAKE_ARGS} -Dtiff-docs=OFF
	DEPENDS libjpeg
)
ADD_GH_GROUP(libtiff)

ExternalProject_Add(libopenjpeg
	URL https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.4.tar.gz
	URL_HASH SHA256=a695fbe19c0165f295a8531b1e4e855cd94d0875d2f88ec4b61080677e27188a
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_CODEC=OFF
	DEPENDS libtiff
)
ADD_GH_GROUP(libopenjpeg)

ExternalProject_Add(lcms2
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.17.tar.gz"
	URL_HASH SHA256=6e6f6411db50e85ae8ff7777f01b2da0614aac13b7b9fcbea66dc56a1bc71418
	CONFIGURE_COMMAND env -S "${CONFIGURE_ENV}" ./configure ${CONFIGURE_ARGS} "--with-tiff=${CMAKE_INSTALL_PREFIX}" "--with-jpeg=${CMAKE_INSTALL_PREFIX}"
	BUILD_IN_SOURCE TRUE
	DEPENDS libtiff libjpeg
)
ADD_GH_GROUP(lcms2)

# cairo?
if (DEFINED ENV{MACOSX_DEPLOYMENT_TARGET} AND $ENV{MACOSX_DEPLOYMENT_TARGET} VERSION_LESS "10.15")
ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-21.12.0.tar.xz"
	URL_HASH SHA256=acb840c2c1ec07d07e53c57c4b3a1ff3e3ee2d888d44e1e9f2f01aaf16814de7
	PATCH_COMMAND patch -p1 -i ${CMAKE_CURRENT_LIST_DIR}/poppler-21.12.0.patch
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_GLIB=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE
	DEPENDS fontconfig freetype lcms2 libjpeg libopenjpeg libpng libtiff
)
else ()
ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-25.10.0.tar.xz"
	URL_HASH SHA256=6b5e9bb64dabb15787a14db1675291c7afaf9387438cc93a4fb7f6aec4ee6fe0
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

# Need to patch the Makefile to not override/clear custom flags, i.e., to allow
# passing them as env variables as there seems to be no easy way to pass complex
# arguments on the command line via ExternalProject_Add (due to weird quotation
# schemes). NB: lua uses plain Makefiles instead of autoconf or cmake, ...
ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.4.8.tar.gz"
	URL_HASH SHA256=4f18ddae154e793e46eeab727c59ef1c0c0c2b744e7b94219710d76f530629ae
	PATCH_COMMAND patch -p1 -i ${CMAKE_CURRENT_LIST_DIR}/lua.patch
	CONFIGURE_COMMAND ""
	BUILD_COMMAND env -S "MYCFLAGS=\"-mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} ${ARCH_FLAGS}\" MYLDFLAGS=\"${ARCH_FLAGS}\"" make
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND make install "INSTALL_TOP=${CMAKE_INSTALL_PREFIX}"
)
ADD_GH_GROUP(lua)
