cmake_minimum_required(VERSION 3.1)

project(tw-deps)

include(ExternalProject)
include(GNUInstallDirs)

set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_SHARED_LIBS=TRUE)
set(MSBUILD_ARGS /t:rebuild /p:Configuration=Release /p:Platform=$(Platform) /p:PlatformToolset=$(PlatformToolset) /p:WindowsTargetPlatformVersion=$(WindowsTargetPlatformVersion))

ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.4.4.tar.gz"
	URL_HASH SHA256=164c7849653b80ae67bec4b7473b884bf5cc8d2dca05653475ec2ed27b9ebf61
	PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lua.cmake <SOURCE_DIR>/CMakeLists.txt
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(hunspell
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.1.tar.gz"
	URL_HASH SHA256=6e3557624c608b3e6525b8bd277706db4f5a857c28fdb3cfa8d0d2b67776da8a
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
	URL "https://github.com/madler/zlib/archive/refs/tags/v1.2.13.tar.gz"
	URL_HASH SHA256=1525952a0a567581792613a9723333d7f8cc20b87a81f920fb8bc7e3f2251428
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(libpng
	URL "https://download.sourceforge.net/libpng/libpng-1.6.38.tar.xz"
	URL_HASH SHA256=b3683e8b8111ebf6f1ac004ebb6b0c975cd310ec469d98364388e9cedbfa68be
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
	URL "https://download.osgeo.org/libtiff/tiff-4.4.0.tar.gz"
	URL_HASH SHA256=917223b37538959aca3b790d2d73aa6e626b688e02dcda272aec24c2f498abed
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libjpeg
)

ExternalProject_Add(lcms2
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.14.tar.gz"
	URL_HASH SHA256=05869269f14e589b0b6d05a76f510c68c67fabb304904d16c6bd818abec80a83
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
	URL https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.0.tar.gz
	URL_HASH SHA256=0333806d6adecc6f7a91243b2b839ff4d2053823634d4f6ed7a59bc87409122a
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_CODEC=OFF
	DEPENDS lcms2 libtiff libpng zlib
)

ExternalProject_Add(freetype
	URL https://download.savannah.gnu.org/releases/freetype/freetype-2.12.1.tar.xz
	URL_HASH SHA256=4766f20157cc4cf0cd292f80bf917f92d1c439b243ac3018debf6b9140c41a7f
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libpng
)

ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-22.12.0.tar.xz"
	URL_HASH SHA256=d9aa9cacdfbd0f8e98fc2b3bb008e645597ed480685757c3e7bc74b4278d15c0
	PATCH_COMMAND git --git-dir=. apply --verbose --ignore-whitespace ${CMAKE_CURRENT_LIST_DIR}/poppler-msvc.patch
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_BOOST=FALSE -DENABLE_CPP=FALSE -DENABLE_UTILS=FALSE -DENABLE_UNSTABLE_API_ABI_HEADERS=TRUE -DBUILD_MANUAL_TESTS=FALSE -DBUILD_QT5_TESTS=FALSE -DWITH_NSS3=FALSE
	DEPENDS freetype lcms2 libjpeg libpng libtiff
)
