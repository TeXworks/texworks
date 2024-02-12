#[==[
## Disclaimer:
 The caller issued instruction
`CMAKE_MINIMUM_REQUIRED(VERSION 3.1)`

## Purpose
Load the Lua package taking care of problems due to
multiple architectures at different locations.
For example on OSX, one may have

* `/opt/local/lib/lualib.dylib` for arm64
* `/usr/local/lib/lualib.dylib` for x86_64

The path is not sufficient to choose between them
for the appropriate architecture.

We add 2 configuration variables, that can be set from the CLI
```
-DTWX_LUA_PATHS="path_1;path_2...path_3"
-DTWX_LUA_NAMES="name_1;name_2...name_3"
```
In our example, one can run
```
cmake -DTWX_LUA_PATHS=/usr/local -DCMAKE_OSX_ARCHITECTURES="x86_64" ..
cmake -DTWX_LUA_PATHS=/opt/local -DCMAKE_OSX_ARCHITECTURES="arm64" ..
```
]==]

if (TWX_LUA_PATHS)
  message("Looking for Lua manually")
  # For a fresh start, remove `CMakeCache.txt` in the build folder
  find_library(
    LUA_LIBRARY
    Lua
    NAMES ${TWX_LUA_NAMES}
    PATHS ${TWX_LUA_PATHS}
    PATH_SUFFIXES lib
    NO_DEFAULT_PATH
  )
  # If we found something, we let the build process check in due time whether it is suitable
  if (LUA_LIBRARY)
    # find headers nearby
    # we assume this library path reads <root>/lib/liblua.dylib
    # We first retrieve <root>,
    # Then look for <root>/include/lua.h or <root>/include/lua/lua.h
    get_filename_component(LUA_INCLUDE_DIR "${LUA_LIBRARY}" DIRECTORY)
    get_filename_component(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}" DIRECTORY)
    set(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}/include")
    if (NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
      set(LUA_INCLUDE_DIR "${LUA_INCLUDE_DIR}/lua")
      if (NOT EXISTS "${LUA_INCLUDE_DIR}/lua.h")
        unset(LUA_LIBRARY)
        unset(LUA_LIBRARY CACHE)
        unset(LUA_INCLUDE_DIR)
        unset(LUA_INCLUDE_DIR CACHE)
      endif ()
    endif ()
  endif ()
  if (NOT LUA_LIBRARY)
    message("No Lua library/header available for:")
    message("  TWX_LUA_PATHS=>${TWX_LUA_PATHS}")
    message("  TWX_LUA_NAMES=>${TWX_LUA_NAMES}")
  endif()
endif()
# default behavior will find the header
# and the library when not already found.
# Anyways, it will setup the associate variables.
find_package(Lua)

#[===[
foreach(
  n
  FOUND
  LIBRARIES
  INCLUDE_DIR
  VERSION_STRING
  VERSION_MAJOR
  VERSION_MINOR
  VERSION_PATCH
)
  message("LUA_${n}=>${LUA_${n}}")
endforeach()
]===]
