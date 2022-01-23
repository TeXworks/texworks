cmake_minimum_required(VERSION 3.1)

project(tw-deps)

include(ExternalProject)
include(GNUInstallDirs)

set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=RELEASE)
set(MSBUILD_ARGS /t:rebuild /p:Configuration=Release /p:Platform=$(Platform) /p:PlatformToolset=$(PlatformToolset) /p:WindowsTargetPlatformVersion=$(WindowsTargetPlatformVersion))

ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.4.3.tar.gz"
	URL_HASH SHA1=1dda2ef23a9828492b4595c0197766de6e784bc7
	PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lua.cmake <SOURCE_DIR>/CMakeLists.txt
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(hunspell
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.0.tar.gz"
	URL_HASH SHA256=bb27b86eb910a8285407cf3ca33b62643a02798cf2eef468c0a74f6c3ee6bc8a
	CONFIGURE_COMMAND ""
	BUILD_COMMAND msbuild msvc\\libhunspell.vcxproj ${MSBUILD_ARGS} /p:Configuration=Release_dll
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_LIBDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different msvc/x64/Release_dll/libhunspell.lib ${CMAKE_INSTALL_FULL_LIBDIR}/hunspell.lib
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_BINDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different msvc/x64/Release_dll/libhunspell.dll ${CMAKE_INSTALL_FULL_BINDIR}/
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_INCLUDEDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different src/hunspell/atypes.hxx ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different src/hunspell/hunspell.h ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different src/hunspell/hunspell.hxx ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different src/hunspell/hunvisapi.h ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different src/hunspell/w_char.hxx ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
)

ExternalProject_Add(zlib
	URL "http://zlib.net/zlib-1.2.11.tar.xz"
	URL_HASH SHA256=4ff941449631ace0d4d203e3483be9dbc9da454084111f97ea0a2114e19bf066
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(libpng
	URL "https://download.sourceforge.net/libpng/libpng-1.6.37.tar.xz"
	URL_HASH SHA256=505e70834d35383537b6491e7ae8641f1a4bed1876dbfe361201fc80868d88ca
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS zlib
)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v9e.tar.gz
	URL_HASH SHA256=4077d6a6a75aeb01884f708919d25934c93305e49f7e3f36db9129320e6f4f3d
	CONFIGURE_COMMAND NMAKE /f makefile.vs setupcopy-v16
	BUILD_COMMAND MSBUILD jpeg.sln ${MSBUILD_ARGS}
	INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_LIBDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different Release/x64/jpeg.lib ${CMAKE_INSTALL_FULL_LIBDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different jpeglib.h jerror.h jconfig.h jmorecfg.h ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
	BUILD_IN_SOURCE TRUE
)

ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.3.0.tar.gz"
	URL_HASH SHA256=0e46e5acb087ce7d1ac53cf4f56a09b221537fc86dfc5daaad1c2e89e1b37ac8
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libjpeg
)

ExternalProject_Add(lcms2
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.12.tar.gz"
	URL_HASH SHA256=e90e77e43be35ec149d39f1ee7fe1c50c73309773a55b13839090f7138fbdcde
	CONFIGURE_COMMAND ""
	BUILD_COMMAND MSBUILD Projects\\VC2019\\lcms2_DLL ${MSBUILD_ARGS}
	BUILD_IN_SOURCE TRUE
	INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_INCLUDEDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different include/lcms2.h ${CMAKE_INSTALL_FULL_INCLUDEDIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_LIBDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different bin/lcms2.lib ${CMAKE_INSTALL_FULL_LIBDIR}
		COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_BINDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different bin/lcms2.dll ${CMAKE_INSTALL_FULL_BINDIR}
)

ExternalProject_Add(libopenjpeg
	URL https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.4.0.tar.gz
	URL_HASH SHA256=8702ba68b442657f11aaeb2b338443ca8d5fb95b0d845757968a7be31ef7f16d
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS lcms2 libtiff libpng zlib
)

ExternalProject_Add(freetype
	URL https://download.savannah.gnu.org/releases/freetype/freetype-2.11.1.tar.xz
	URL_HASH SHA256=3333ae7cfda88429c97a7ae63b7d01ab398076c3b67182e960e5684050f2c5c8
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libpng
)

ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-22.01.0.tar.xz"
	URL_HASH SHA256=7d3493056b5b86413e5c693c2cae02c5c06cd8e618d14c2c31e2c84b67b2313e
	PATCH_COMMAND git --git-dir=. apply --verbose --ignore-whitespace ${CMAKE_CURRENT_LIST_DIR}/poppler-msvc.patch
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE -DBUILD_MANUAL_TESTS=FALSE -DBUILD_QT5_TESTS=FALSE
	DEPENDS freetype lcms2 libjpeg libpng libtiff
)
