# Top-level build file for [TeXworks][tw], a simple editor for TeX and friends.
#
# This buildsystem uses [CMake][cmake] and consists of the following files:
#
# <pre>
#   TeXworks
#   ├── <a href="CMakeLists.html">CMakeLists.txt</a>
#   ├── src
#   │   └── <a href="src/CMakeLists.html">CMakeLists.txt</a>
#   ├── plugins-src
#   │   ├── TWLuaPlugin
#   │   │   └── <a href="plugins-src/TWLuaPlugin/CMakeLists.html">CMakeLists.txt</a>
#   │   └── TWPythonPlugin
#   │       └── <a href="plugins-src/TWPythonPlugin/CMakeLists.html">CMakeLists.txt</a>
#   └── CMake
#       └── packaging
#           ├── <a href="CMake/packaging/CMakeLists.html">CMakeLists.txt</a>
#           └── mac
#               └── <a href="CMake/packaging/mac/MacPackagingTasks.cmake.html">MacPackagingTasks.cmake.in</a>
# </pre>
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
#   [tw]: https://github.com/TeXworks/texworks


# Setup and User Options
# ======================

# Set project name.
PROJECT(TeXworks)

# We require CMake v2.8 or greater because it is the first version that
# provides support for certain parts of Qt, such as the ScriptTools.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.6)
CMAKE_POLICY(VERSION 2.8.6)

# Silence warning about linking to qtmain.lib statically on Windows
IF(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
ENDIF()

# Silence warning about using @rpath on OS X.
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

SET(CMAKE_COLOR_MAKEFILE ON)
# Always add the current source and binary directories to the header include
# path when compiling.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC TRUE)

IF(WIN32 AND MINGW)
  # Ensure that no cpp flags are passed to windres, the Windows resource compiler.
  # At least with MinGW 4 on Windows, that would cause problems
  SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> <SOURCE> <OBJECT>")
ENDIF()


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

# Make sure we have up-to-date git commit infos
IF ( NOT CMAKE_CROSSCOMPILING )
	IF ( WIN32 )
		EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/getGitRevInfo.bat WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		ADD_CUSTOM_TARGET(GitRev ALL COMMAND ${CMAKE_SOURCE_DIR}/getGitRevInfo.bat WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMENT "Update git commit info")
	ELSE ( WIN32 )
		EXECUTE_PROCESS(COMMAND ${CMAKE_SOURCE_DIR}/getGitRevInfo.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
		ADD_CUSTOM_TARGET(GitRev ALL COMMAND ${CMAKE_SOURCE_DIR}/getGitRevInfo.sh WORKING_DIRECTORY ${CMAKE_SOURCE_DIR} COMMENT "Update git commit info")
	ENDIF ( WIN32 )
ENDIF ( NOT CMAKE_CROSSCOMPILING)
MESSAGE( STATUS "Git commit info updated" )

# Recover git commit info from `src/GitRev.h`.
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/GitRev.h TeXworks_GIT_INFO)
LIST(GET TeXworks_GIT_INFO 1 TeXworks_GIT_COMMIT)
STRING(REGEX REPLACE "#define GIT_COMMIT_HASH \"([a-f0-9]+)\"" "\\1" TeXworks_GIT_HASH "${TeXworks_GIT_COMMIT}")
LIST(GET TeXworks_GIT_INFO 2 TeXworks_GIT_COMMIT)
STRING(REGEX REPLACE "#define GIT_COMMIT_DATE \"([-+:0-9 Z]+)\"" "\\1" TeXworks_GIT_DATE "${TeXworks_GIT_COMMIT}")


# Declare Project Options
# -----------------------

# These are options that users can set at configure-time to determine how the
# application is built and what functionalities it will have. Here we declare
# these options by setting default values.

SET(TW_BUILD_ID "personal" CACHE STRING "A distribution-specific identifier for TeXworks")

# Build with shared libs by default
IF ( NOT DEFINED BUILD_SHARED_LIBS )
  SET(BUILD_SHARED_LIBS ON)
ENDIF ()
OPTION(BUILD_SHARED_LIBS "Build with shared libs?" ${BUILD_SHARED_LIBS})
MARK_AS_ADVANCED(BUILD_SHARED_LIBS)

# Build shared plugins by default
IF ( NOT DEFINED BUILD_SHARED_PLUGINS )
  SET(BUILD_SHARED_PLUGINS ${BUILD_SHARED_LIBS})
ENDIF()
OPTION(BUILD_SHARED_PLUGINS "Build shared plugins?" ${BUILD_SHARED_PLUGINS})
MARK_AS_ADVANCED(BUILD_SHARED_PLUGINS)

OPTION(PREFER_BUNDLED_SYNCTEX "Use SyncTeX bundled with TeXworks even if a shared library is found?" OFF)
MARK_AS_ADVANCED(PREFER_BUNDLED_SYNCTEX)

SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE PATH "Path to place libraries in")
SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR} CACHE PATH "Path to place programs in")
MARK_AS_ADVANCED(CMAKE_LIBRARY_OUTPUT_DIRECTORY CMAKE_RUNTIME_OUTPUT_DIRECTORY)

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
OPTION(WITH_LUA "Build TeXworks Lua plugin?" ON)
OPTION(WITH_PYTHON "Build TeXworks Python plugin?" OFF)

# On OS X we default to linking against the Python libraries provided by Apple
# even if other Pythons are available. This helps when building
# re-distributable `.app` packages. By disabling this option, a Mac user can
# link against whatever Python they wish for a personal build.
IF ( APPLE )
  OPTION(USE_SYSTEM_PYTHON "Link against Python Framework distributed with OS X?" ON)
ENDIF ()

IF (UNIX AND NOT APPLE)
  SET(TeXworks_HELP_DIR share/doc/texworks-help CACHE PATH "Path where TeXworks looks for help files")
  SET(TeXworks_DIC_DIR /usr/share/myspell/dicts CACHE PATH "Path where TeXworks looks for spellchecking dictionaries")
  SET(TeXworks_DOCS_DIR share/doc/texworks CACHE PATH "Path in which documentation files are installed")
  SET(TeXworks_ICON_DIR share/pixmaps CACHE PATH "Path in which icons are installed")
  SET(TeXworks_MAN_DIR share/man/man1 CACHE PATH "Path in which manpages are installed")
  SET(TeXworks_DESKTOP_DIR share/applications CACHE PATH "Path in which .desktop files are installed")
  SET(TeXworks_APPDATA_DIR share/appdata CACHE PATH "Path in which .appdata.xml files are installed")
  MARK_AS_ADVANCED(TeXworks_HELP_DIR)
  MARK_AS_ADVANCED(TeXworks_DIC_DIR)
  MARK_AS_ADVANCED(TeXworks_DOCS_DIR)
  MARK_AS_ADVANCED(TeXworks_ICON_DIR)
  MARK_AS_ADVANCED(TeXworks_MAN_DIR)
  MARK_AS_ADVANCED(TeXworks_DESKTOP_DIR)
  MARK_AS_ADVANCED(TeXworks_APPDATA_DIR)
  IF (EXISTS ${TeXworks_SOURCE_DIR}/manual/)
    INSTALL(DIRECTORY ${TeXworks_SOURCE_DIR}/manual/ DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_HELP_DIR}/ OPTIONAL)
  ENDIF()
ENDIF()

set(CMAKE_CXX_STANDARD 11)

# Dependency Configuration
# ========================

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as hunspell.
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules ${CMAKE_MODULE_PATH})

set(DESIRED_QT_VERSION "" CACHE STRING "Pick a version of Qt to use: 4 or 5")
MARK_AS_ADVANCED(DESIRED_QT_VERSION)

# Check for Qt5 by default
IF (NOT DESIRED_QT_VERSION OR DESIRED_QT_VERSION MATCHES 5)
  find_package(Qt5Widgets QUIET)
  find_package(Qt5Gui QUIET)
  find_package(Qt5UiTools QUIET)
  find_package(Qt5Concurrent QUIET)
  find_package(Qt5Script QUIET)
  find_package(Qt5ScriptTools QUIET)
  find_package(Qt5Xml QUIET)
  find_package(Qt5LinguistTools QUIET)
  IF ( UNIX AND NOT APPLE )
    find_package(Qt5DBus QUIET)
  ENDIF ()

  IF(Qt5Widgets_FOUND AND Qt5Core_FOUND AND Qt5Gui_FOUND AND Qt5UiTools_FOUND AND Qt5Concurrent_FOUND AND Qt5Script_FOUND AND Qt5ScriptTools_FOUND AND Qt5Xml_FOUND AND Qt5LinguistTools_FOUND AND (NOT UNIX OR APPLE OR Qt5DBus_FOUND))
    SET(QT5_FOUND TRUE)
    SET(QT_LIBRARIES Qt5::UiTools Qt5::ScriptTools Qt5::Script Qt5::Concurrent Qt5::Xml ${Qt5DBus_LIBRARIES} Qt5::Widgets Qt5::Gui Qt5::Core)

    # Note: Qt5 only sets Qt5Widgets_VERSION, etc., but not QT_VERSION_MAJOR,
    # etc. which is used here.
    string(REGEX REPLACE "^([0-9]+).*$" "\\1" QT_VERSION_MAJOR "${Qt5Widgets_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_MINOR  "${Qt5Widgets_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_PATCH "${Qt5Widgets_VERSION}")
  ELSE()
    SET(QT5_FOUND FALSE)
  ENDIF()

  IF( QT5_FOUND AND WIN32 AND NOT BUILD_SHARED_LIBS )
    # For static Windows builds, we also need to pull in the Qt5 Platform
    # support library, which is not exposed to CMake properly, unfortunately
    GET_TARGET_PROPERTY(QT_LIB_DIR "${Qt5Widgets_LIBRARIES}" LOCATION)
    GET_FILENAME_COMPONENT(QT_LIB_DIR "${QT_LIB_DIR}" PATH)

    FIND_LIBRARY(Qt5QWindows_LIBRARIES qwindows
      HINTS "${QT_LIB_DIR}/../plugins/platforms"
    )

    # Starting with Qt 5.8, Qt5PlatformSupport was modularized
    if (QT_VERSION_MAJOR EQUAL 5 AND QT_VERSION_MINOR LESS 8)
      FIND_LIBRARY(Qt5Platform_LIBRARIES Qt5PlatformSupport
        HINTS "${QT_LIB_DIR}"
      )
      LIST(INSERT QT_LIBRARIES 0 ${Qt5Platform_LIBRARIES} ${Qt5QWindows_LIBRARIES})
    else()
      FIND_LIBRARY(Qt5FontDatabaseSupport_LIBRARIES Qt5FontDatabaseSupport
        HINTS "${QT_LIB_DIR}"
      )
      FIND_LIBRARY(Qt5EventDispatcherSupport_LIBRARIES Qt5EventDispatcherSupport
        HINTS "${QT_LIB_DIR}"
      )
      FIND_LIBRARY(Qt5ThemeSupport_LIBRARIES Qt5ThemeSupport
        HINTS "${QT_LIB_DIR}"
      )
      LIST(INSERT QT_LIBRARIES 0 ${Qt5QWindows_LIBRARIES} ${Qt5FontDatabaseSupport_LIBRARIES} ${Qt5EventDispatcherSupport_LIBRARIES} ${Qt5ThemeSupport_LIBRARIES})
    endif()
    ADD_DEFINITIONS(-DSTATIC_QT5)
  ENDIF()

ENDIF()
# If Qt5 is not found (or not desired), check for Qt4
IF (DESIRED_QT_VERSION MATCHES 4 OR NOT QT5_FOUND)
  # Declare Qt libraries required by this project.
  SET (QT_COMPONENTS
      QtCore
      QtGui
      QtUiTools
      QtScript
      QtScriptTools
      QtXml
  )

  IF ( UNIX AND NOT APPLE )
    LIST(APPEND QT_COMPONENTS QtDbus)
  ENDIF ()

  # Locate Qt. `INCLUDE(UseQt4)` brings in a set of macros that helps us deal
  # with Qt-specific tasks such as compiling resources or running `moc`.
  FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED)
  INCLUDE(UseQt4)
ENDIF()

IF (NOT QT4_FOUND AND NOT QT5_FOUND)
  MESSAGE(FATAL_ERROR "Could not find required Qt. Please install Qt (see http://qt-project.org/) and/or adjust DESIRED_QT_VERSION.")
ENDIF()

# Expose the major version number of Qt to the preprocessor. This is necessary
# to include the correct Qt headers (as QTVERSION is not defined before any Qt
# headers are included)
ADD_DEFINITIONS(-DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})
INCLUDE( TranslationMacros )



FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
IF ( NOT PREFER_BUNDLED_SYNCTEX )
  FIND_PACKAGE(Synctex QUIET)
ENDIF ( NOT PREFER_BUNDLED_SYNCTEX )

# Aggregate library names and include directories into variables for easy
# access.
SET(TeXworks_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${HUNSPELL_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

IF ( SYNCTEX_FOUND AND NOT PREFER_BUNDLED_SYNCTEX )
  LIST(APPEND TeXworks_INCLUDE_DIRS ${SYNCTEX_INCLUDE_DIR})
ENDIF ()

SET(TeXworks_LIB_DIRS ${QT_LIBRARY_DIR})

set(TEXWORKS_ADDITIONAL_LIBS "" CACHE STRING "Additional libraries not found by CMake")
MARK_AS_ADVANCED(TEXWORKS_ADDITIONAL_LIBS)


SET(TeXworks_LIBS
  ${HUNSPELL_LIBRARIES}
  ${QT_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${TEXWORKS_ADDITIONAL_LIBS}
)

IF ( SYNCTEX_FOUND AND NOT PREFER_BUNDLED_SYNCTEX )
  LIST(APPEND TeXworks_LIBS ${SYNCTEX_LIBRARIES})
ENDIF ()


# Configure Optional Dependencies
# -------------------------------

IF ( WITH_LUA )
  FIND_PACKAGE(Lua)
ENDIF()

IF ( WITH_PYTHON )
  IF ( USE_SYSTEM_PYTHON )
    SET(PYTHON_LIBRARIES "-F/System/Library/Frameworks -framework Python" CACHE PATH "Python library.")
    SET(PYTHON_INCLUDE_DIR "/System/Library/Framework/Python.framework/Headers" CACHE PATH "Python framework.")
    MARK_AS_ADVANCED(PYTHON_LIBRARIES)
    MARK_AS_ADVANCED(PYTHON_INCLUDE_DIR)
    SET(PYTHONLIBS_FOUND TRUE)
  ELSE ()
    # **NOTE**
    # In order to find the correct version of 'PythonLibs', it seems that we need to run 'FIND_PACKAGE(PythonInterp)' firstly.
    # In order to find the correct version of 'PythonInterp', we need to set 'PYTHONHOME' environment variable
    FIND_PACKAGE(PythonInterp)
    FIND_PACKAGE(PythonLibs)
  ENDIF ()
ENDIF()

IF ( LUA_FOUND AND WITH_LUA AND NOT ${BUILD_SHARED_PLUGINS})
  ADD_DEFINITIONS(-DQT_STATICPLUGIN -DSTATIC_LUA_SCRIPTING_PLUGIN)
ENDIF ()

IF ( PYTHONLIBS_FOUND AND WITH_PYTHON AND NOT ${BUILD_SHARED_PLUGINS})
  ADD_DEFINITIONS(-DQT_STATICPLUGIN -DSTATIC_PYTHON_SCRIPTING_PLUGIN)
ENDIF ()


# Update Header Templates
# -----------------------

# Some header files contain components that must be dynamically generated. For
# example, `getDefaultBinPaths.sh` is a script that tries to figure out the
# appropriate path to TeX binaries on UNIX-like systems.
IF ( UNIX AND NOT CMAKE_CROSSCOMPILING )
  IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/src/DefaultBinaryPaths.h )
    MESSAGE(STATUS "Generating DefaultBinaryPaths.h")

    # FIXME: Make getDefaultBinPaths.sh cross-compilation aware
    EXECUTE_PROCESS(
      COMMAND ${TeXworks_SOURCE_DIR}/getDefaultBinPaths.sh
      WORKING_DIRECTORY ${TeXworks_SOURCE_DIR}
    )
  ENDIF ()
ENDIF ()

# Building
# ========

# Build modules
ADD_SUBDIRECTORY(modules)
INCLUDE_DIRECTORIES(${MODULE_INCLUDE_DIRS})

# Build Plugins
# -------------

# On OS X, the plugins should live inside the application bundle.
IF ( APPLE )
  SET(TeXworks_PLUGIN_DIR ${PROJECT_NAME}.app/Contents/PlugIns)
  # Plugins are build as shared libraries that contain undefined symbols.
  # Pass `-undefined dynamic_lookup` so that the Apple linker does not
  # freak out about this.
  SET(CMAKE_MODULE_LINKER_FLAGS
    "${CMAKE_MODULE_LINKER_FLAGS} -undefined dynamic_lookup")
ELSE ()
  # Set the plugin installation path. This is a good default for UNIX-like
  # systems, but is not appropriate for Windows.
  SET(TeXworks_PLUGIN_DIR lib/texworks CACHE PATH "Path where TeXworks looks for plugins")
  MARK_AS_ADVANCED(TeXworks_PLUGIN_DIR)
ENDIF ()

# Build main TeXworks application
# -------------------------------
ADD_SUBDIRECTORY(src)

# Build Plugins
# -------------

# Build scripting language plugins if the required libraries are available.
IF ( LUA_FOUND AND WITH_LUA )
  ADD_SUBDIRECTORY(${TeXworks_SOURCE_DIR}/plugins-src/TWLuaPlugin)
ENDIF ()

IF ( PYTHONLIBS_FOUND AND WITH_PYTHON )
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
if (QT_VERSION_MAJOR EQUAL 4)
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
elseif (QT_VERSION_MAJOR EQUAL 5)
  if (WIN32)
    get_target_property(QT_QPLATFORM_PLUGIN_RELEASE Qt5::QWindowsIntegrationPlugin LOCATION_Release)
  elseif (APPLE)
    get_target_property(QT_QPLATFORM_PLUGIN_RELEASE Qt5::QCocoaIntegrationPlugin LOCATION_Release)
  endif ()
  SET(QT_PLUGINS "${QT_QPLATFORM_PLUGIN_RELEASE}")
endif()

# The file `CMake/packaging/CMakeLists.txt` controls the execution of tasks
# specific to preparing binaries for packaging on a given platform. This script
# is invoked via `ADD_SUBDIRECTORY` so that it executes after TeXworks and its
# components are built and installed. You cannot fixup an application bundle
# before it has been created...
ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/CMake/packaging)

# Set CPack variables.
SET(CPACK_PACKAGE_VERSION_MAJOR ${TeXworks_VER_MAJOR})
SET(CPACK_PACKAGE_VERSION_MINOR ${TeXworks_VER_MINOR})
SET(CPACK_PACKAGE_VERSION_PATCH ${TeXworks_VER_PATCH})
SET(CPACK_PACKAGE_VERSION ${TeXworks_VERSION})

SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CPACK_PACKAGE_VERSION}-${TeXworks_GIT_HASH}")

# Configure Drag N' Drop installer.
IF ( APPLE )
  SET(MAC_PACK_BITS "${CMAKE_SOURCE_DIR}/CMake/packaging/mac")
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


# Summary
# =======

# This section displays a nice configuration summary for the user.

# These macros borrowed from the Poppler CMake scripts. They add some nice
# formatting to configuration info.
MACRO(CONFIG_INFO what value)
  STRING(LENGTH ${what} length_what)
  MATH(EXPR left_char "35 - ${length_what}")
  SET(blanks)
  FOREACH(_i RANGE 1 ${left_char})
    SET(blanks "${blanks} ")
  ENDFOREACH()

  MESSAGE("  ${what}:${blanks} ${value}")
ENDMACRO()

MACRO(CONFIG_YESNO what enabled)
  IF(${enabled})
    SET(enabled_string "yes")
  ELSE(${enabled})
    SET(enabled_string "no")
  ENDIF()

  CONFIG_INFO("${what}" "${enabled_string}")
ENDMACRO()

# Print out configuration summary.
MESSAGE("TeXworks has been configured:\n")

CONFIG_INFO("Version" ${TeXworks_VERSION})
CONFIG_INFO("Git Commit" "${TeXworks_GIT_HASH} (${TeXworks_GIT_DATE})")
CONFIG_INFO("Build ID" ${TW_BUILD_ID})
CONFIG_INFO("Qt version" ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH})
CONFIG_INFO("Compiler optimization" ${CMAKE_BUILD_TYPE})
MESSAGE("")

CONFIG_YESNO("Lua scripting plugin" LUA_FOUND)
CONFIG_YESNO("Python scripting plugin" PYTHONLIBS_FOUND)
IF( APPLE )
  CONFIG_YESNO("  OS X system python" USE_SYSTEM_PYTHON)
ENDIF()
MESSAGE("")

MESSAGE("  TeXworks will be installed to:")
MESSAGE("      ${CMAKE_INSTALL_PREFIX}")
MESSAGE("")
