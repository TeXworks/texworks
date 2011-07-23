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
MESSAGE(STATUS "TeXworks Major version: ${TeXworks_VERSION}")

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

IF( APPLE AND CMAKE_INSTALL_PREFIX MATCHES "/usr/local" )
  # CMake chooses /usr/local by default, but Mac users probably want things to
  # install in /Applications
  SET( CMAKE_INSTALL_PREFIX "/Applications" )
ENDIF()

SET(WITH_LUA TRUE CACHE BOOL "Build TeXworks Lua plugin?")


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

IF ( UNIX )
  LIST(APPEND QT_COMPONENTS QtDbus)
ENDIF ()

FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED)
INCLUDE(UseQt4)

FIND_PACKAGE(Fontconfig REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
FIND_PACKAGE(Poppler REQUIRED)
FIND_PACKAGE(PopplerQT4 REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

# Collect information on required include and library files.
SET(TeXworks_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${HUNSPELL_INCLUDE_DIR}
  ${POPPLERQT4_INCLUDE_DIR}
  ${FONTCONFIG_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

SET(TeXworks_LIB_DIRS
  ${QT_LIBRARY_DIRS}
  ${HUNSPELL_LIBRARY_DIRS}
  ${POPPLER_LIBRARY_DIRS}
  ${POPPLERQT4_LIBRARY_DIRS}
  ${FONTCONFIG_LIBRARY_DIRS}
)

SET(TeXworks_LIBS
  ${QT_LIBRARIES}
  ${POPPLER_LIBRARIES}
  ${POPPLERQT4_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
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
  SET(TeXworks_PLUGIN_DIR ${PROJECT_NAME}.app/Contents/plugins)
ENDIF ()


# Script Plugins
IF ( LUA51_FOUND )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWLuaPlugin)
ENDIF ()


#===============================================================================
#                                  Packaging
#===============================================================================

SET(CPACK_PACKAGE_VERSION_MAJOR ${TeXworks_VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TeXworks_VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TeXworks_VER_PATCH})
SET(CPACK_PACKAGE_VERSION ${TeXworks_VERSION})

SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CPACK_PACKAGE_VERSION}-${TeXworks_SVN_REV}-${TeXworks_GIT_REV}")

IF ( APPLE )
  # Nice way would be to put this in cmake/Modules with a descriptive filename
  # like MacBundler.cmake. However, it has to be brought in using
  # ADD_SUBDIRECTORY so that the INSTALL() directives are triggered after the
  # TeXworks `src` directory is processed. Otherwise, CMake tries to run
  # install_name_tool on a .app that hasn't been created yet.
  ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/cmake/packaging/mac)

  SET(CPACK_GENERATOR DragNDrop)
ENDIF ()

INCLUDE(CPack)

