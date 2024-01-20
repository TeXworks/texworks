# Top-level build file for [TeXworks][tw], a simple editor for TeX and friends.
#
# This buildsystem uses [CMake][cmake] and consists of the following files:
#
# <pre>
#   TeXworks
#   ├── <a href="CMakeLists.html">CMakeLists.txt</a>
#   ├── src
#   │   └── <a href="src/CMakeLists.html">CMakeLists.txt</a>
#   ├── modules
#   │   ├── <a href="modules/CMakeLists.html">CMakeLists.txt</a>
#   │   ├── synctex
#   │   │   └── <a href="modules/synctex/CMakeLists.html">CMakeLists.txt</a>
#   │   └── QtPDF (documented separately)
#   ├── plugins-src
#   │   ├── TWLuaPlugin
#   │   │   └── <a href="plugins-src/TWLuaPlugin/CMakeLists.html">CMakeLists.txt</a>
#   │   └── TWPythonPlugin
#   │       └── <a href="plugins-src/TWPythonPlugin/CMakeLists.html">CMakeLists.txt</a>
#   ├── unit-tests
#   │   └── <a href="unit-tests/CMakeLists.html">CMakeLists.txt</a>
#   └── CMake
#       └── packaging
#           ├── <a href="CMake/packaging/CMakeLists.html">CMakeLists.txt</a>
#           └── mac
#               └── <a href="CMake/packaging/mac/MacPackagingTasks.cmake.html">MacPackagingTasks.in.cmake</a>
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

# CMake 3.1 significantly improves support for imported targets, Qt5, c++11, etc.
CMAKE_MINIMUM_REQUIRED(VERSION 3.1...3.28)

# Set project name.
PROJECT(TeXworks)

SET(CMAKE_COLOR_MAKEFILE ON)
# Always add the current source and binary directories to the header include
# path when compiling.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)
SET(CMAKE_AUTOMOC TRUE)
SET(CMAKE_AUTORCC TRUE)
SET(CMAKE_AUTOUIC TRUE)

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as hunspell.
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules ${PROJECT_SOURCE_DIR}/modules/QtPDF/CMake/Modules ${CMAKE_MODULE_PATH})
SET(
  CMAKE_MODULE_PATH
  ${PROJECT_SOURCE_DIR}/CMake/Include
  ${CMAKE_MODULE_PATH}
)

IF(WIN32 AND MINGW)
  # Ensure that no cpp flags are passed to windres, the Windows resource compiler.
  # At least with MinGW 4 on Windows, that would cause problems
  SET(CMAKE_RC_COMPILE_OBJECT "<CMAKE_RC_COMPILER> -O coff <DEFINES> <SOURCE> <OBJECT>")
ENDIF()

if (MSVC)
	add_compile_options("$<$<C_COMPILER_ID:MSVC>:/utf-8>")
	add_compile_options("$<$<CXX_COMPILER_ID:MSVC>:/utf-8>")
endif (MSVC)


# Determine Version Numbers
# -------------------------

# The values recovered here are used in the filenames of packaged builds and
# influence parts of the application such as the "About" dialog.

#include(TeXworksVersion)
#
#GetTeXworksVersion()
#UpdateGitRevInfo()
#GetGitRevInfo()

# Recover canonical (x.y.z) version number from `src/TWVersion.h`.
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MAJOR REGEX "VER_MAJOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MAJOR ${TeXworks_VER_MAJOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_MINOR REGEX "VER_MINOR")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_MINOR ${TeXworks_VER_MINOR})
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/TWVersion.h TeXworks_VER_PATCH REGEX "VER_BUGFIX")
STRING(REGEX MATCH "([0-9]+)" TeXworks_VER_PATCH ${TeXworks_VER_PATCH})
SET(TeXworks_VERSION ${TeXworks_VER_MAJOR}.${TeXworks_VER_MINOR}.${TeXworks_VER_PATCH})

# Make sure we have up-to-date git commit infos
execute_process(COMMAND "${CMAKE_COMMAND}" "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/src" -P "${PROJECT_SOURCE_DIR}/CMake/Modules/getGitRevInfo.cmake" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}")
add_custom_target(GitRev ALL "${CMAKE_COMMAND}" "-DOUTPUT_DIR=${CMAKE_CURRENT_BINARY_DIR}/src" -P "${PROJECT_SOURCE_DIR}/CMake/Modules/getGitRevInfo.cmake" WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}" COMMENT "Update git commit info")

# Recover git commit info from `src/GitRev.h`.
FILE(STRINGS ${CMAKE_CURRENT_BINARY_DIR}/src/GitRev.h TeXworks_GIT_INFO)
LIST(GET TeXworks_GIT_INFO 1 TeXworks_GIT_COMMIT)
STRING(REGEX REPLACE "#define GIT_COMMIT_HASH \"([a-f0-9]+\\*?)\"" "\\1" TeXworks_GIT_HASH "${TeXworks_GIT_COMMIT}")
LIST(GET TeXworks_GIT_INFO 2 TeXworks_GIT_COMMIT)
STRING(REGEX REPLACE "#define GIT_COMMIT_DATE \"([-+:0-9TZ]+)\"" "\\1" TeXworks_GIT_DATE "${TeXworks_GIT_COMMIT}")


# Declare Project Options
# -----------------------

# These are options that users can set at configure-time to determine how the
# application is built and what functionalities it will have. Here we declare
# these options by setting default values.

SET(TW_BUILD_ID "personal" CACHE STRING "A distribution-specific identifier for TeXworks")

# Build with shared libs by default
OPTION(BUILD_SHARED_LIBS "Build with shared libs?" ON)
MARK_AS_ADVANCED(BUILD_SHARED_LIBS)

# Build shared plugins by default
OPTION(BUILD_SHARED_PLUGINS "Build shared plugins?" ${BUILD_SHARED_LIBS})
MARK_AS_ADVANCED(BUILD_SHARED_PLUGINS)

OPTION(PREFER_BUNDLED_SYNCTEX "Use SyncTeX bundled with TeXworks even if a shared library is found?" OFF)
MARK_AS_ADVANCED(PREFER_BUNDLED_SYNCTEX)

OPTION(WITH_TESTS "build unit tests" ON)
IF (WITH_TESTS)
  ENABLE_TESTING(TRUE)
ENDIF (WITH_TESTS)

OPTION(WITH_COVERAGE "build with lcov coverage support" OFF)
IF (WITH_COVERAGE)
  IF (NOT (CMAKE_BUILD_TYPE STREQUAL "Debug" AND WITH_TESTS))
    MESSAGE(WARNING "Coverage support is intended to be used with Debug builds and testing enabled")
  ENDIF ()
  INCLUDE(CodeCoverage)
  set(COVERAGE_COMPILER_FLAGS "${COVERAGE_COMPILER_FLAGS} -fno-inline")
  APPEND_COVERAGE_COMPILER_FLAGS()
  IF (UNIX)
    SET(COVERAGE_LCOV_EXCLUDES '/usr/*')
  ENDIF (UNIX)
  SETUP_TARGET_FOR_COVERAGE_LCOV(NAME coverage EXECUTABLE "${CMAKE_CTEST_COMMAND}")
ENDIF (WITH_COVERAGE)

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
  SET(TeXworks_DIC_DIR /usr/share/hunspell:/usr/share/myspell/dicts CACHE PATH "Path where TeXworks looks for spellchecking dictionaries")
  SET(TeXworks_DOCS_DIR share/doc/texworks CACHE PATH "Path in which documentation files are installed")
  SET(TeXworks_ICON_DIR share/icons/hicolor CACHE PATH "Path in which icons are installed")
  SET(TeXworks_MAN_DIR share/man/man1 CACHE PATH "Path in which manpages are installed")
  SET(TeXworks_DESKTOP_DIR share/applications CACHE PATH "Path in which .desktop files are installed")
  SET(TeXworks_APPDATA_DIR share/metainfo CACHE PATH "Path in which .appdata.xml files are installed")
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

set (CMAKE_CXX_STANDARD 11)

# Dependency Configuration
# ========================

if (QT_DEFAULT_MAJOR_VERSION EQUAL 6)
	# Check for Qt6
	find_package(Qt6 REQUIRED COMPONENTS Core Core5Compat Widgets Gui UiTools Concurrent Xml LinguistTools Qml)
	set(QT_LIBRARIES Qt6::Core Qt6::Core5Compat Qt6::Widgets Qt6::Gui Qt6::UiTools Qt6::Concurrent Qt6::Xml Qt6::Qml)

	if (UNIX AND NOT APPLE)
	  find_package(Qt6 REQUIRED COMPONENTS DBus)
	  list(APPEND QT_LIBRARIES Qt6::DBus)
	endif ()

	if (WITH_TESTS)
	  find_package(Qt6 OPTIONAL_COMPONENTS Test QUIET)
	  if (NOT Qt6Test_FOUND)
		set(WITH_TESTS OFF)
	  else ()
		list(APPEND QT_LIBRARIES Qt6::Test)
	  endif ()
	endif ()

	if (APPLE)
		# Inspired by https://github.com/MaximAlien/macdeployqt/blob/bc9c0ba199f323a42e3f1cc04d4b66e3e59ac995/macdeployqt/shared.cpp
		if (QT_PLUGIN_PATH)
			set(_pluginDir "${QT_PLUGIN_PATH}")
		else ()
			get_target_property(_pluginDir Qt6::Widgets LOCATION)
			get_filename_component(_pluginDir "${_pluginDir}" REALPATH)
			get_filename_component(_pluginDir "${_pluginDir}" DIRECTORY)
			set(_pluginDir "${_pluginDir}/../../../../share/qt/plugins")
			get_filename_component(_pluginDir "${_pluginDir}" REALPATH)
		endif ()

		foreach (_lib IN ITEMS platforms/qcocoa styles/qmacstyle)
			get_filename_component(_lib_name ${_lib} NAME)
			get_filename_component(_lib_dir ${_lib} DIRECTORY)
			find_library(_plugin_${_lib_name} NAMES ${_lib_name} HINTS ${_pluginDir} PATH_SUFFIXES ${_lib_dir})
			if (NOT _plugin_${_lib_name})
				message(FATAL_ERROR "Could not find plugin ${_lib} in ${_pluginDir}")
			else ()
				list(APPEND QT_PLUGINS "${_plugin_${_lib_name}}")
			endif ()
		endforeach ()

		message(STATUS "QT_PLUGINS = ${QT_PLUGINS}")
	endif ()

	set(QT_VERSION_MAJOR "${Qt6_VERSION_MAJOR}")
	set(QT_VERSION_MINOR "${Qt6_VERSION_MINOR}")
	set(QT_VERSION_PATCH "${Qt6_VERSION_PATCH}")
else ()
	# Check for Qt5
	find_package(Qt5 REQUIRED COMPONENTS Core Widgets Gui UiTools Concurrent Xml LinguistTools Qml)
	set(QT_LIBRARIES Qt5::Core Qt5::Widgets Qt5::Gui Qt5::UiTools Qt5::Concurrent Qt5::Xml Qt5::Qml)

	find_package(Qt5 OPTIONAL_COMPONENTS Script ScriptTools)
	if (Qt5Script_FOUND AND Qt5ScriptTools_FOUND)
		list(APPEND QT_LIBRARIES Qt5::ScriptTools Qt5::Script)
		set(WITH_QTSCRIPT ON)
	endif()

	if (UNIX AND NOT APPLE)
	  find_package(Qt5 REQUIRED COMPONENTS DBus)
	  list(APPEND QT_LIBRARIES Qt5::DBus)
	endif ()

	if (WITH_TESTS)
	  find_package(Qt5 OPTIONAL_COMPONENTS Test)
	  if (NOT Qt5Test_FOUND)
		set(WITH_TESTS OFF)
	  else ()
		list(APPEND QT_LIBRARIES Qt5::Test)
	  endif ()
	endif ()

	if (WIN32 AND NOT BUILD_SHARED_LIBS)
	  find_package(Qt5WindowsPlatformSupport)
	  list(APPEND QT_LIBRARIES Qt5::WindowsPlatformSupport)
	endif ()

	set(QT_VERSION_MAJOR "${Qt5_VERSION_MAJOR}")
	set(QT_VERSION_MINOR "${Qt5_VERSION_MINOR}")
	set(QT_VERSION_PATCH "${Qt5_VERSION_PATCH}")
endif ()

# Expose the major version number of Qt to the preprocessor. This is necessary
# to include the correct Qt headers (as QTVERSION is not defined before any Qt
# headers are included)
ADD_DEFINITIONS(-DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})



FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
FIND_PACKAGE(Synctex QUIET)

# Aggregate library names and include directories into variables for easy
# access.

SET(TeXworks_LIBS
  SyncTeX::synctex
  Hunspell::hunspell
  ${QT_LIBRARIES}
  ZLIB::ZLIB
  ${TEXWORKS_ADDITIONAL_LIBS}
)



# Configure Optional Dependencies
# -------------------------------

IF ( WITH_LUA )
  find_package(TwxLua)
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

include(GenerateDefaultBinPaths)
GenerateDefaultBinPaths()

# Building
# ========

if (MSVC)
	set(WARNING_OPTIONS /W4)
else ()
	set(WARNING_OPTIONS -Wall -Wpedantic -Wextra -Wconversion -Wold-style-cast -Woverloaded-virtual)
	if (NOT "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}" VERSION_LESS "5.6.0")
		# Old Qt versions were heavily using 0 instead of nullptr, giving lots
		# of false positives
		list(APPEND WARNING_OPTIONS -Wzero-as-null-pointer-constant)
	endif ()
endif ()

# Build modules
ADD_SUBDIRECTORY(modules)

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
ELSEIF (WIN32)
  # Put plugins alongside the main app in the root installation folder on Windows
  SET(TeXworks_PLUGIN_DIR . CACHE PATH "Path where TeXworks looks for plugins")
  MARK_AS_ADVANCED(TeXworks_PLUGIN_DIR)
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

# Tests
# -----
IF (WITH_TESTS)
  ADD_SUBDIRECTORY(unit-tests)
ENDIF (WITH_TESTS)


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
# be bundled into packages. (Required on APPLE for a consistent macOS look&feel)
foreach (_target ${Qt5Gui_PLUGINS} ${Qt5Widgets_PLUGINS})
	get_target_property(_path ${_target} LOCATION_Release)
	list(APPEND QT_PLUGINS ${_path})
endforeach()

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

# Configure Drag N' Drop installer.
IF ( APPLE )
  SET(MAC_PACK_BITS "${CMAKE_SOURCE_DIR}/CMake/packaging/mac")
  SET(CPACK_GENERATOR DragNDrop)
  SET( CPACK_DMG_BACKGROUND_IMAGE "${MAC_PACK_BITS}/texworks_dmg_background.png" )
  # Configure an AppleScript for applying a nice window layout to Drag N' Drop disk
  # image.
  set(CPACK_DMG_DS_STORE_SETUP_SCRIPT "${MAC_PACK_BITS}/DMGSetup.scpt")
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

macro(CONFIG_VERSION lib version)
  CONFIG_INFO("  ${lib}" "${version}")
endmacro()

# Print out configuration summary.
MESSAGE("TeXworks has been configured (CMake ${CMAKE_VERSION}, ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}):\n")

message("  Version info")
CONFIG_VERSION("${PROJECT_NAME}" "${TeXworks_VERSION} (${TeXworks_GIT_HASH}, ${TeXworks_GIT_DATE})")
# Hunspell does not have a reliable way to determine its version
CONFIG_YESNO("  Hunspell" "${HUNSPELL_FOUND}")
IF ( WITH_LUA )
  CONFIG_VERSION("Lua" "${LUA_VERSION_STRING}")
ENDIF()
if (WITH_PYTHON)
  CONFIG_VERSION("Python" "${PYTHON_VERSION_STRING}")
endif()
CONFIG_VERSION("Qt" ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH})
CONFIG_VERSION("SyncTeX" "${SYNCTEX_VERSION_STRING}")
CONFIG_VERSION("zlib" "${ZLIB_VERSION_STRING}")
message("")

message("  Scripting")
CONFIG_YESNO("  ECMA scripting" ON)
CONFIG_YESNO("  QtScript scripting" WITH_QTSCRIPT)
CONFIG_YESNO("  Lua scripting plugin" LUA_FOUND)
CONFIG_YESNO("  Python scripting plugin" PYTHONLIBS_FOUND)
message("")

CONFIG_INFO("Build ID" ${TW_BUILD_ID})
CONFIG_INFO("Compiler optimization" ${CMAKE_BUILD_TYPE})
IF( APPLE )
  CONFIG_YESNO("  OS X system python" USE_SYSTEM_PYTHON)
ENDIF()
MESSAGE("")

MESSAGE("  TeXworks will be installed to:")
MESSAGE("      ${CMAKE_INSTALL_PREFIX}")
MESSAGE("")
