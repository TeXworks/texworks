#[===============================================[/*
This is part of the TWX build and test system.
See https://github.com/TeXworks/texworks
(C)  JL 2023
*//** @file
@brief  Base material everyone should include

See @ref CMake/README.md.

Usage:
```
include (
  "${CMAKE_CURRENT_LIST_DIR}/<...>/CMake/Include/Base.cmake"
  NO_POLICY_SCOPE
)
```
This is called from a `CMakeFiles.txt` which perfectly knows where it lives
and can replace `<...>` with the proper subpath.

__Do not forget `NO_POLICY_SCOPE`!__

After a new `project(...)` instruction is executed, issue
```
include ( Base )
```
Output:
- `TWX_DIR`
- `twx_target_include_src()`

Includes module `TwxCoreLib`.
Also includes module `CMakeParseArguments` until this is built in.

Implementation details:
This script may be called in various situations.
- from the main `CMakeLists.txt` at configuration time
- from a target at build time
- from another script in `-P` mode, at either time.

In any cases, the global variables above are expected to point to
the same location. For `TWX_DIR` it is easy because its location
relative to the various `.cmake` script files is well known
at some early point.
*/
/*#]===============================================]

# Full include only once
if ( DEFINED TWX_IS_BASED )
# This has already been included
  if ( TWX_CONFIG_VERBOSE )
    message ( STATUS "TwxBase: ${CMAKE_PROJECT_NAME}(${TWX_DIR})" )
  endif ()

	set ( CMAKE_CXX_STANDARD 11 )
	# Tell CMake to run moc and friends when necessary:
	set ( CMAKE_AUTOMOC ON )
	set ( CMAKE_AUTOUIC ON )
	set ( CMAKE_AUTORCC ON )
	# As moc files are generated in the binary dir, tell CMake
	# to always look for includes there:
	set ( CMAKE_INCLUDE_CURRENT_DIR ON )

  SET(CMAKE_COLOR_MAKEFILE ON)

# Minor changes
  set ( TWX_NAME_CURRENT CMAKE_PROJECT_NAME )
  if ( NOT "${CMAKE_PROJECT_NAME}" STREQUAL "${PROJECT_NAME}" )
    set ( TWX_PROJECT_IS_ROOT OFF )
  endif ()

  return ()
endif ()

set ( TWX_IS_BASED ON )

# Next is run only once per cmake session.
# A different process can run this however on its own.

# We load the policies as soon as possible
# Before using any higher level cmake command
include (
  "${CMAKE_CURRENT_LIST_DIR}/TwxBasePolicy.cmake"
  NO_POLICY_SCOPE
)

set (TWX_PROJECT_IS_ROOT ON)

#[=======[ Paths setup
This is called from various locations.
We cannot assume that `PROJECT_SOURCE_DIR` always represent
the same location, in particular when called from a module
or a sub code unit. The same holds for `CMAKE_SOURCE_DIR`.
`TWX_DIR` is always "at the top" because it is defined
relative to this included file.
#]=======]
get_filename_component (
  TWX_DIR
  "${CMAKE_CURRENT_LIST_DIR}/../.."
  REALPATH
)

#[=======[ setup `CMAKE_MODULE_PATH`
Make the contents of `CMake/Include` and `CMake/Modules` available.
The former contains tools and utilities whereas
the latter only contains modules at a higher level.
We also rely on QtPDF embeded modules.
]=======]
list (
  INSERT CMAKE_MODULE_PATH 0
  "${TWX_DIR}/CMake/Include"
  "${TWX_DIR}/CMake/Modules"
  "${TWX_DIR}/modules/QtPDF/CMake/Modules"
)

# ANCHOR: twx_target_include_src
#[=======[*/
/*! @fn twx_target_include_src(target)
Include `src` directories.

Add the main `.../src` directory as well the `src`
subdirectory of the project binary directory to
the list of include directories of the given target.
*/
twx_target_include_src(target) {}
/*#]=======]
function ( twx_target_include_src target_ )
  twx_assert_non_void ( PROJECT_BINARY_DIR )
  target_include_directories (
    ${target_}
    PRIVATE ${TWX_DIR}/src ${PROJECT_BINARY_DIR}/src
  )
endfunction ()

include ( TwxCoreLib )

include ( CMakeParseArguments )

message ( STATUS "TwxBase: initialize(${TWX_DIR})" )

#[=======[
*/
#]=======]
