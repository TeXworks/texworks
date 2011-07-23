#===============================================================================
#                                Project Info
#===============================================================================
PROJECT(TeXworks)
# Support for certain parts of Qt, such as the Script Tools, was only added in
# CMake v.2.8
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_COLOR_MAKEFILE ON)

IF ( NOT APPLE )
  MESSAGE( WARNING
    "This CMake build process has only been tested on Mac OS X."
    "\nIt may not work on other systems, but patches are welcome!"
  )
ENDIF ()

# Version Numbers
# ---------------

# Canonical version number
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MAJOR REGEX "VER_MAJOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MAJOR ${TeXworks_VER_MAJOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MINOR REGEX "VER_MINOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MINOR ${TeXworks_VER_MINOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_PATCH REGEX "VER_BUGFIX")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_PATCH ${TeXworks_VER_PATCH})
SET(TeXworks_VERSION ${TeXworks_VER_MAJOR}.${TeXworks_VER_MINOR}.${TeXworks_VER_PATCH})
MESSAGE(STATUS "TeXworks Version: ${TeXworks_VERSION}")

# SVN revision
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/SvnRev.h TeXworks_SVN_REV LIMIT_COUNT 1)
STRING(REGEX MATCH "([0-9]+)" TeXworks_SVN_REV ${TeXworks_SVN_REV})
MESSAGE(STATUS "TeXworks SVN Revision: ${TeXworks_SVN_REV}")

# GIT revision---in case I upgrade the build script and make new binaries
# before the SVN revision advances. This could be removed if the CMake build
# process is ever imported into the main SVN repo.
EXECUTE_PROCESS(COMMAND git describe --always OUTPUT_VARIABLE TeXworks_GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE)
MESSAGE(STATUS "TeXworks Git Revision: ${TeXworks_GIT_REV}")

# Project Options
# ---------------

SET(TW_BUILD_ID "personal" CACHE STRING "A distribution-specific identifier for TeXworks")

IF( APPLE AND CMAKE_INSTALL_PREFIX MATCHES "/usr/local" )
  # CMake chooses /usr/local by default, but Mac users probably want things to
  # install in /Applications
  SET( CMAKE_INSTALL_PREFIX "/Applications" )
ENDIF()

IF ( NOT CMAKE_BUILD_TYPE )
  # Do an optimized release build by default
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF ()

SET(WITH_LUA TRUE CACHE BOOL "Build TeXworks Lua plugin?")

SET(WITH_PYTHON TRUE CACHE BOOL "Build TeXworks Python plugin?")
IF ( APPLE )
  # Reccomended for build re-distributable .app packages. For personal builds,
  # use whatever Python you wish.
  SET(USE_SYSTEM_PYTHON TRUE CACHE BOOL "Link against Python Framework distributed with OS X")
ENDIF ()


#===============================================================================
#                                Configuration
#===============================================================================
# Contains CMake scripts that help locate required components
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})

# Find required packages.
SET (QT_COMPONENTS
    QtCore
    QtGui
    QtUiTools
    QtScript
    QtScriptTools
    QtXML
)

IF ( UNIX AND NOT APPLE )
  LIST(APPEND QT_COMPONENTS QtDbus)
ENDIF ()

FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED)
INCLUDE(UseQt4)

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
FIND_PACKAGE(Poppler REQUIRED)

IF ( POPPLER_NEEDS_FONTCONFIG )
  # The only thing poppler should need is the location of the include
  # directories. The library loader should be able to find libfontconfig on
  # standard search paths.
  FIND_PACKAGE(Fontconfig REQUIRED)
ENDIF ()

# Collect information on required include and library files.
SET(TeXworks_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${HUNSPELL_INCLUDE_DIR}
  ${POPPLER_QT4_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)
IF ( POPPLER_NEEDS_FONTCONFIG )
  LIST(APPEND TeXworks_INCLUDE_DIRS ${FONTCONFIG_INCLUDE_DIR})
ENDIF ()

SET(TeXworks_LIB_DIRS ${QT_LIBRARY_DIR})

SET(TeXworks_LIBS
  ${QT_LIBRARIES}
  ${POPPLER_LIBRARIES}
  ${HUNSPELL_LIBRARIES}
  ${ZLIB_LIBRARIES}
)

# Configure Optional Dependencies
# -------------------------------
IF ( WITH_LUA )
  # CMake 2.8 has seperate find scripts for Lua 5.1 and Lua 5.0. Going with the
  # most recent one for now.
  FIND_PACKAGE(Lua51)
ENDIF()

IF ( WITH_PYTHON )
  IF ( USE_SYSTEM_PYTHON )
    SET(PYTHON_LIBRARIES "-F/System/Library/Frameworks -framework Python" CACHE PATH "Python library.")
    SET(PYTHON_INCLUDE_DIR "/System/Library/Framework/Python.framework/Headers" CACHE PATH "Python framework.")
    MARK_AS_ADVANCED(PYTHON_LIBRARIES)
    MARK_AS_ADVANCED(PYTHON_INCLUDE_DIR)
    SET(PYTHONLIBS_FOUND TRUE)
  ELSE ()
    # For some reason, FindPythonLibs.cmake always seems to default to
    # /System/Library/Frameworks/Python.framework/Headers as the include path
    # on OS X.
    FIND_PACKAGE(PythonLibs)
  ENDIF ()
ENDIF()

# Configure Template Files
# ------------------------
IF ( UNIX )
  IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/src/DefaultBinaryPaths.h )
    MESSAGE(STATUS "Generating DefaultBinaryPaths.h")

    EXECUTE_PROCESS(
      COMMAND ${TeXworks_SOURCE_DIR}/getDefaultBinPaths.sh
      WORKING_DIRECTORY ${TeXworks_SOURCE_DIR}
    )
  ENDIF ()
ENDIF ()


#===============================================================================
#                                  Building
#===============================================================================

# Build  main TeXworks app
# ------------------------
ADD_SUBDIRECTORY(src)

# Build Plugins
# -------------

# Default install path for UNIX-like systems. Definitely not appropriate for Windows.
SET(TeXworks_PLUGIN_DIR lib)

IF ( APPLE )
  SET(TeXworks_PLUGIN_DIR ${PROJECT_NAME}.app/Contents/PlugIns)
ENDIF ()


# Script Plugins
IF ( LUA51_FOUND )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWLuaPlugin)
ENDIF ()

IF ( PYTHONLIBS_FOUND )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWPythonPlugin)
ENDIF ()


#===============================================================================
#                                  Packaging
#===============================================================================

# cmake/packaging/CMakeLists.txt contains packaging-specifc tasks. These are
# invoked via ADD_SUBDIRECTORY so that they execute after TeXworks and its
# plugins are built.
SET(QT_PLUGINS
  ${QT_QTACCESSIBLEWIDGETS_PLUGIN_RELEASE}
  ${QT_QCNCODECS_PLUGIN_RELEASE}
  ${QT_QJPCODECS_PLUGIN_RELEASE}
  ${QT_QKRCODECS_PLUGIN_RELEASE}
  ${QT_QTWCODECS_PLUGIN_RELEASE}
  ${QT_QGIF_PLUGIN_RELEASE}
  ${QT_QJPEG_PLUGIN_RELEASE}
  ${QT_QTIFF_PLUGIN_RELEASE}
)

ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/cmake/packaging)

SET(CPACK_PACKAGE_VERSION_MAJOR ${TeXworks_VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TeXworks_VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TeXworks_VER_PATCH})
SET(CPACK_PACKAGE_VERSION ${TeXworks_VERSION})

SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CPACK_PACKAGE_VERSION}-${TeXworks_SVN_REV}-${TeXworks_GIT_REV}")

IF ( APPLE )
  SET(CPACK_GENERATOR DragNDrop)
ENDIF ()

INCLUDE(CPack)

