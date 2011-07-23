# Top-level build file for [TeXworks][tw], a simple editor for TeX and friends.
#
# This buildsystem uses [CMake][cmake] and consists of the following files:
#
#     TeXworks
#     ├── CMakeLists.txt
#     ├── src
#     │   └── CMakeLists.txt
#     ├── plugins-src
#     │   ├── TWLuaPlugin
#     │   │   └── CMakeLists.txt
#     │   └── TWPythonPlugin
#     │       └── CMakeLists.txt
#     └── CMake
#         └── packaging
#             └── CMakeListst.txt
#
# Using CMake as a buildsystem offers some nice advantages:
#
#   * Supports component discovery and configuration for all three major
#     operating systems and can set TeXworks up to be built using GNU
#     Makefiles, XCode or Visual Studio.
#
#   * Includes CPack, a tool which simplifies the task of packaging the
#     TeXworks app for release after it has been built. CPack can generate
#     Drag N' Drop installers for the Mac, Nullsoft installers for Windows and
#     `.deb` or `.rpm` packages for Linux.
#
# Comments in the CMake files follow [Markdown][md] formatting conventions.
# This convention allows the `CMakeLists.txt` files to be passed through a tool
# such as [Docco][docco] to create a nicely annotated README for the benefit of
# future developers and maintainers.
#
#   [cmake]: http://www.cmake.org
#   [docco]: http://jashkenas.github.com/docco
#   [md]: http://daringfireball.net/projects/markdown/
#   [tw]: http://code.google.com/p/texworks


# Setup and User Options
# ======================

# Set project name.
PROJECT(TeXworks)

# We require CMake v2.8 or greater because it is the first version that
# provides support for certain parts of Qt, such as the ScriptTools.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_COLOR_MAKEFILE ON)
# Always add the current source and binary directories to the header include
# path when compiling.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)

# This message is probably superfluous.
IF ( NOT APPLE )
  MESSAGE( WARNING
    "This CMake build process has only been tested on Mac OS X."
    "\nIt may not work on other systems, but patches are welcome!"
  )
ENDIF ()

# Determine Version Numbers
# -------------------------

# The values recovered here are used in the filenames of packaged builds and
# influence parts of the application such as the "About" dialog.

# Recover canonical (x.y.z) version number from `src/TWVersion.h`.
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MAJOR REGEX "VER_MAJOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MAJOR ${TeXworks_VER_MAJOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MINOR REGEX "VER_MINOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MINOR ${TeXworks_VER_MINOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_PATCH REGEX "VER_BUGFIX")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_PATCH ${TeXworks_VER_PATCH})
SET(TeXworks_VERSION ${TeXworks_VER_MAJOR}.${TeXworks_VER_MINOR}.${TeXworks_VER_PATCH})
MESSAGE(STATUS "TeXworks Version: ${TeXworks_VERSION}")

# Recover SVN revision from `src/SvnRev.h`.
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/SvnRev.h TeXworks_SVN_REV LIMIT_COUNT 1)
STRING(REGEX MATCH "([0-9]+)" TeXworks_SVN_REV ${TeXworks_SVN_REV})
MESSAGE(STATUS "TeXworks SVN Revision: ${TeXworks_SVN_REV}")

# Recover GIT revision: first 7 digets of the current Git SHA. Currently, this
# is only used in the filenames of packaged releases. If this CMake buildsystem
# ever gets merged back into the main SVN repository, this code can be removed.
EXECUTE_PROCESS(COMMAND git log HEAD -1 --pretty=%h OUTPUT_VARIABLE TeXworks_GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE)
MESSAGE(STATUS "TeXworks Git Revision: ${TeXworks_GIT_REV}")

# Declare Project Options
# -----------------------

# These are options that users can set at configure-time to determine how the
# application is built and what functionalities it will have. Here we declare
# these options by setting default values.

SET(TW_BUILD_ID "personal" CACHE STRING "A distribution-specific identifier for TeXworks")

# On UNIX, CMake installs to `/usr/local` by default. However, Mac users
# will probably find `/Applications` a more convenient default.
IF( APPLE AND CMAKE_INSTALL_PREFIX MATCHES "/usr/local" )
  SET( CMAKE_INSTALL_PREFIX "/Applications" )
ENDIF()

# Do an optimized release build by default
IF ( NOT CMAKE_BUILD_TYPE )
  SET(CMAKE_BUILD_TYPE "Release")
ENDIF ()

# Options controlling the creation of scripting language plugins.
SET(WITH_LUA TRUE CACHE BOOL "Build TeXworks Lua plugin?")
SET(WITH_PYTHON TRUE CACHE BOOL "Build TeXworks Python plugin?")

# On OS X we default to linking against the Python libraries provided by Apple
# even if other Pythons are available. This helps when building
# re-distributable `.app` packages. By disabling this option, a Mac user can
# link against whatever Python they wish for a personal build.
IF ( APPLE )
  SET(USE_SYSTEM_PYTHON TRUE CACHE BOOL "Link against Python Framework distributed with OS X")
ENDIF ()


# Dependency Configuration
# ========================

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as Poppler.
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules ${CMAKE_MODULE_PATH})

# Declare Qt libraries required by this project.
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

# Locate Qt. `INCLUDE(UseQt4)` brings in a set of macros that helps us deal
# with Qt-specific tasks such as compiling resources or running `moc`.
FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED)
INCLUDE(UseQt4)

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
FIND_PACKAGE(Poppler REQUIRED)

# The only thing Poppler should need is the location of the include directories
# in order to access header files. The library loader should be able to find
# libfontconfig on standard search paths.
IF ( POPPLER_NEEDS_FONTCONFIG )
  FIND_PACKAGE(Fontconfig REQUIRED)
ENDIF ()

# Aggregate library names and include directories into variables for easy
# access.
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

# CMake 2.8 has separate find scripts for Lua 5.1 and Lua 5.0. We assume that
# the most recent Lua version is installed.
IF ( WITH_LUA )
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
    # **NOTE** For some reason, FindPythonLibs.cmake always seems to default to
    # `/System/Library/Frameworks/Python.framework/Headers` as the include path
    # on OS X.
    FIND_PACKAGE(PythonLibs)
  ENDIF ()
ENDIF()

# Update Header Templates
# -----------------------

# Some header files contain components that must be dynamically generated. For
# example, `getDefaultBinPaths.sh` is a script that tries to figure out the
# appropriate path to TeX binaries on UNIX-like systems.
IF ( UNIX )
  IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/src/DefaultBinaryPaths.h )
    MESSAGE(STATUS "Generating DefaultBinaryPaths.h")

    EXECUTE_PROCESS(
      COMMAND ${TeXworks_SOURCE_DIR}/getDefaultBinPaths.sh
      WORKING_DIRECTORY ${TeXworks_SOURCE_DIR}
    )
  ENDIF ()
ENDIF ()


# Building
# ========

# Build main TeXworks application
# -------------------------------
ADD_SUBDIRECTORY(src)

# Build Plugins
# -------------

# Set the plugin installation path. This is a good default for UNIX-like
# systems, but is not appropriate for Windows.
SET(TeXworks_PLUGIN_DIR lib)

# On OS X, the plugins should live inside the application bundle.
IF ( APPLE )
  SET(TeXworks_PLUGIN_DIR ${PROJECT_NAME}.app/Contents/PlugIns)
ENDIF ()


# Build scripting language plugins if the required libraries are available.
IF ( LUA51_FOUND )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWLuaPlugin)
ENDIF ()

IF ( PYTHONLIBS_FOUND )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWPythonPlugin)
ENDIF ()


# Packaging
# =========

# This section is responsible for executing all tasks that run when a user
# invokes `cpack` or `make package` after executing their platform's equivalent
# of of `make`. The packaging tasks are responsible for two things:
#
#  * Applying transformations to the compiled binaries such that they can be
#    distributed to another machine.
#
#  * Gathering all binaries and libraries and packaging them into an archive
#    suitable for distribution such as a Windows installer or Mac disk image
#    file.

# Gather all libraries related to Qt plugins used by TeXworks so that they can
# be bundled into packages.
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

# The file `cmake/packaging/CMakeLists.txt` controls the execution of tasks
# specific to preparing binaries for packaging on a given platform. This script
# is invoked via `ADD_SUBDIRECTORY` so that it executes after TeXworks and its
# components are built and installed. You cannot fixup an application bundle
# before it has been created...
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/cmake/packaging)

# Set CPack variables.
SET(CPACK_PACKAGE_VERSION_MAJOR ${TeXworks_VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TeXworks_VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TeXworks_VER_PATCH})
SET(CPACK_PACKAGE_VERSION ${TeXworks_VERSION})

SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CPACK_PACKAGE_VERSION}-${TeXworks_SVN_REV}-${TeXworks_GIT_REV}")

# Configure Drag N' Drop installer.
IF ( APPLE )
  SET(MAC_PACK_BITS "${CMAKE_SOURCE_DIR}/cmake/packaging/mac")
  SET(CPACK_GENERATOR DragNDrop)
  SET( CPACK_DMG_BACKGROUND_IMAGE "${MAC_PACK_BITS}/texworks_dmg_background.png" )

  # Configure an AppleScript for applying a nice window layout to Drag N' Drop disk
  # image.
  CONFIGURE_FILE( ${MAC_PACK_BITS}/set_dmg_layout.scpt.in
    ${PROJECT_BINARY_DIR}/set_dmg_layout.scpt
    @ONLY
  )
ENDIF ()

INCLUDE(CPack)

