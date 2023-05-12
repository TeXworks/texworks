# About TeXworks folder /CMake

(Work in progress)

* `Modules` contains custom package loaders. Each file inside is included
  with the `find_package` instruction. (In progress)
* `packaging` contains material to build distribution packages.
* `Include` contains various tools to be used with instruction `include`.
  None will load a package, it may eventually provide tools to load a package.

Various `CMakeLists.txt`

This folder contains utilities to build various `CMakeLists.txt`.

## Problem
The whole TeXworks code is divided into partially independent modules superseeded by a primary `CMakeLists.txt` at the top level.
In order to build and test the various modules separately,
we don't always want to start from the top, but from a directory below, mainly one of the modules.
Then we need to share some configuration settings and tools,
at least the `C++` compiler version and `CMake` policy.
This will be achieved by the inclusion of various `.cmake` files contained in this library.


## Shared preamble
It is a minimal set of configuration settings and tools.
The `TwxBase.cmake` should be included by any main `CMakeLists.txt` at the very top with for example:
```
include(
  "${CMAKE_CURRENT_LIST_DIR}/<...>/CMake/Include/Base.cmake"
  NO_POLICY_SCOPE
)
```
where `<...>` is replaced with the approriate number of `..` components to indicate a path relative to the directory of the containing `CMakeLists.txt`.

In general, auxiliary `CMakeLists.txt` loaded after an `add_subdirectory(...)` instruction don't need to include `TwxBase.cmake`.
However, some `CMakeLists.txt`, like in modules, may be either main or auxiliary: we need to differentiate the situation.
The first time `TwxBase.cmake` is loaded after a `project(...)` instruction,
the global variable `TWX_PROJECT_IS_ROOT` is set to a truthy value.
After any subsequent attempt to load `TwxBase.cmake` after a `project(...)` instruction,
this global variable is set to false.
Such `CMakeLists.txt` will start with
```
include(
  "${CMAKE_CURRENT_LIST_DIR}/<...>/CMake/Include/Base.cmake"
  NO_POLICY_SCOPE
)
if (TWX_PROJECT_IS_ROOT)
  <do some configuration as main>
else ()
  <do some configuration as secondary>
endif ()
```

The other `.cmake` files shall not include `TwxBase.cmake`,
except the tools.

### Global variables
All of them are prefixed with `TWX_`.
`TWX_DIR` is the path of the directory containing all the sources. Other variable mimic the directory hierarchy with the exact case sensitive folder names. These folder names may change in the future for a better readability, using a global variable will make any change easier.

Other variables are defined by included `.cmake` files.

Beware of the scopes while defining new variables.

### `include`
Once the base is loaded, we can use `include(...)` instructions without specifying a full file path, instead we just give the module name. The subfolder `CMake/Include` is used for that.

Moreover, `find_package(...)` will look for modules into subfolder `CMake/Modules` in addition to standard locations.

The `Include` directory contains global tools and functions, whereas the `Module` directory really contains module related material.

## Guard
In order to prevent some `<file name>.cmake` file of this folder to be included more than once, we can use a trick similar to `.h` macro guards.
The very first `CMake` instructions are sometimes
```
if(DEFINED TWX_GUARD_CMake_Include_<file name>)
  return()
endif()
set(TWX_GUARD_CMake_Include_<file name>)
```
At least it guards from including twice the same file at the same scope level.
`TWX_GUARD_CMake_Include_<file name>` may be replaced by anything more relevant.

## Coding style
It is a weak convention to prefix global variables by `TWX_`, macros and functions or local variables by `twx_`. When inside a function,
a leading or trailing `_` denotes local variables.

The global commands defined here are prefixed with `twx_`,
which clearly indicates that they are not standard commands. 
Names follow modern cmake case relative standards,
according to this quote from `CMake` maintener Brad King
  | Ancient CMake versions required upper-case commands.
  | Later command names became case-insensitive.
  | Now the preferred style is lower-case.

## Available `.cmake` files description

* `TwxBase`: everyone primary `CMakeLists.txt` must include this.
* `TwxBasePolicy`: the policy settings.

File names starting with `Twx` indicate a stronger bound with `TeXworks`.
Others indicate more general contents.

## Various configuration flags used

* WIN32 AND MINGW
* MSVC
