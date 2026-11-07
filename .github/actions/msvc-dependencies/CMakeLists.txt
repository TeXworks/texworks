cmake_minimum_required(VERSION 3.5...4.3)

project(tw-deps)

include(ExternalProject)
include(GNUInstallDirs)

# Set target platform for compiling MSVC dependencies
if(CMAKE_GENERATOR_PLATFORM)
    set(PLATFORM ${CMAKE_GENERATOR_PLATFORM})
else()
    set(PLATFORM x64)
endif()

set(CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX} -DCMAKE_BUILD_TYPE=RELEASE -DBUILD_SHARED_LIBS=TRUE)
set(MSBUILD_ARGS /t:rebuild /p:Configuration=Release /p:Platform=$(PLATFORM) /p:PlatformToolset=$(PlatformToolset) /p:WindowsTargetPlatformVersion=$(WindowsTargetPlatformVersion))

ExternalProject_Add(lua
	URL "https://www.lua.org/ftp/lua-5.5.1.tar.gz"
	URL_HASH SHA256=1c4b4068d67061f2a2231ad2b5422e77acea1487ea9890f6320af614f4373dce
	PATCH_COMMAND ${CMAKE_COMMAND} -E copy_if_different ${CMAKE_CURRENT_LIST_DIR}/lua.cmake <SOURCE_DIR>/CMakeLists.txt
	CMAKE_ARGS ${CMAKE_ARGS} -DCMAKE_POLICY_VERSION_MINIMUM=3.5
)


ExternalProject_Add(hunspell
	URL "https://github.com/hunspell/hunspell/archive/refs/tags/v1.7.6.tar.gz"
	URL_HASH SHA256=4fa9c52a5337fb0a3e03f3212b6372d0159eac3d1a3e6e938e8897ff3d9c3d0c
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(zlib
	URL "https://github.com/madler/zlib/archive/refs/tags/v1.4.tar.gz"
	URL_HASH SHA256=4331355048e286908260fdbeaa0b8cf75ceaf2907c75855a093a6487c7032aaa
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(libpng
	URL "https://github.com/glennrp/libpng/archive/refs/tags/v1.6.59.tar.gz"
	URL_HASH SHA256=2540302a1844ad2b2b501977abecfa850f265f97b78f065a712ab4074a89f5b5
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS zlib
)

ExternalProject_Add(libjpeg
	URL https://ijg.org/files/jpegsrc.v10.tar.gz
	URL_HASH SHA256=8b9eaa13242690ebd03e1728ab1edf97a81a78ed6e83624d493655f31ac95ab5
	CONFIGURE_COMMAND NMAKE /f makefile.vs setupcopy-v16
	BUILD_COMMAND MSBUILD jpeg.sln ${MSBUILD_ARGS}
	INSTALL_COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_INSTALL_FULL_LIBDIR}
		COMMAND ${CMAKE_COMMAND} -E copy_if_different Release/${PLATFORM}/jpeg.lib ${CMAKE_INSTALL_FULL_LIBDIR}/
		COMMAND ${CMAKE_COMMAND} -E copy_if_different jpeglib.h jerror.h jconfig.h jmorecfg.h ${CMAKE_INSTALL_FULL_INCLUDEDIR}/
	BUILD_IN_SOURCE TRUE
)

ExternalProject_Add(libtiff
	URL "https://download.osgeo.org/libtiff/tiff-4.7.2.tar.gz"
	URL_HASH SHA256=672bd7d10aee4606171afb864f3570b83340f6a33e2c186dc0512f7145ffdf6a
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libjpeg
)

ExternalProject_Add(lcms2
	URL "https://github.com/mm2/Little-CMS/archive/refs/tags/lcms2.20.tar.gz"
	URL_HASH SHA256=49908a4492fe7fc886517d636474bd953ebe698d38ecd3ee5862dfa308dd43ae
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
	URL https://github.com/uclouvain/openjpeg/archive/refs/tags/v2.5.4.tar.gz
	URL_HASH SHA256=a695fbe19c0165f295a8531b1e4e855cd94d0875d2f88ec4b61080677e27188a
	CMAKE_ARGS ${CMAKE_ARGS} -DBUILD_CODEC=OFF
	DEPENDS lcms2 libtiff libpng zlib
)

ExternalProject_Add(brotli
	URL "https://github.com/google/brotli/archive/refs/tags/v1.2.1.tar.gz"
	URL_HASH SHA256=98d35d11a78dc453aebb7d10c3748f3f73ff78c8267c0dc697b8631f67bf7e8f
	CMAKE_ARGS ${CMAKE_ARGS}
)

ExternalProject_Add(freetype
	URL "https://github.com/freetype/freetype/archive/refs/tags/VER-2-15-0.tar.gz"
	URL_HASH SHA256=87c7f1f8699b91e89f773f7dcfa505b044e1dae67f62a55d4e29c3afdcac4fb8
	CMAKE_ARGS ${CMAKE_ARGS}
	DEPENDS libpng brotli
)

ExternalProject_Add(poppler
	URL "https://poppler.freedesktop.org/poppler-26.11.0.tar.xz"
	URL_HASH SHA256=41c32680160addf5a8dd7a830e4869c01e9f6bee1abb91e44fabd1ba9d7828fb
	PATCH_COMMAND git --git-dir=. apply --verbose --ignore-whitespace ${CMAKE_CURRENT_LIST_DIR}/poppler-msvc.patch
	CMAKE_ARGS ${CMAKE_ARGS} -DENABLE_QT5=OFF -DENABLE_BOOST=OFF -DENABLE_CPP=OFF -DENABLE_UTILS=OFF -DENABLE_UNSTABLE_API_ABI_HEADERS=ON -DBUILD_MANUAL_TESTS=OFF -DBUILD_QT5_TESTS=OFF -DENABLE_NSS3=OFF -DENABLE_GPGME=OFF -DENABLE_LIBCURL=OFF -DENABLE_LIBTIFF=OFF -DENABLE_LIBPNG=OFF
	DEPENDS freetype lcms2 libjpeg brotli
)
