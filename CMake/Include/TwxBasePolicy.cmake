#[===============================================[/*
This is part of the TWX build and test system.
https://github.com/TeXworks/texworks
(C)  JL 2023
*//** @file
@brief Policy of policies

Automatically loaded by the `TwxBase` module.

Actual policy version: 3.1

- CMP0020: Silence warning about linking to qtmain.lib statically on Windows
- CMP0042: Silence warning about using @@rpath on OS X.
- CMP0054: Only interpret if () arguments as variables or keywords when unquoted.
- CMP0058: Silence warning about ninja custom command byproducts
- CMP0071: Let AUTOMOC and AUTOUIC process GENERATED files.
- CMP0072: Prefer newer OpenGL libraries over legacy ones
- CMP0077: Silence warning about option() treating variables differently on the first run
- CMP0135: CMake 3.24 and above prefers to set the timestamps of all extracted contents to the time of the extraction.
- CMP0140: CMake 3.25: the return() command checks its parameters.

*//*
#]===============================================]

if ( NOT TWX_IS_BASED )
  message( FATAL_ERROR "Base not loaded" )
endif ()

# NB: This file MUST be included with NO_POLICY_COPE
# Otherwise all the changes below won't live after the end

# CMake 3.1 significantly improves support for imported targets, Qt5, c++11, etc.
cmake_policy ( VERSION 3.1 )

# Silence warning about linking to qtmain.lib statically on Windows
if ( POLICY CMP0020 )
  cmake_policy ( SET CMP0020 NEW )
endif ()

# Silence warning about using @rpath on OS X.
if ( POLICY CMP0042 )
  cmake_policy ( SET CMP0042 NEW )
endif ()

# Only interpret if () arguments as variables or keywords when unquoted.
if ( POLICY CMP0054 )
  cmake_policy ( SET CMP0054 NEW )
endif ()

# Silence warning about ninja custom command byproducts
if ( POLICY CMP0058 )
  cmake_policy ( SET CMP0058 NEW )
endif ()

# Let AUTOMOC and AUTOUIC process GENERATED files.
if ( POLICY CMP0071 )
  cmake_policy ( SET CMP0071 NEW )
endif ()

# Prefer newer OpenGL libraries over legacy ones
if ( POLICY CMP0072 )
  cmake_policy ( SET CMP0072 NEW )
endif ()

# Silence warning about option() treating variables differently on the first run
if ( POLICY CMP0077 )
  cmake_policy ( SET CMP0077 NEW )
endif ()

# CMake 3.24 and above prefers to set the timestamps of all extracted contents to the time of the extraction.
if ( POLICY CMP0135 )
  cmake_policy ( SET CMP0135 NEW )
endif ()

# CMake 3.25: the return() command checks its parameters.
if ( POLICY CMP0140 )
  cmake_policy ( SET CMP0140 NEW )
endif ()

# ANCHOR: twx_export
#[=======[
*//**
Convenient shortcut to export a variable to the parent scope.
@param variable the name of the variable to be exported.
*/
twx_export(variable){}
/*
#]=======]
macro ( twx_export what_ )
  set ( ${what_} ${${what_}} PARENT_SCOPE )
endmacro ()

#[=======[ Nota bene
On older CMake, to apply policies locally use
```
cmake_policy ( PUSH )
cmake_policy ( POP )
```
Modern CMake (since 3.25) use `block`.
]=======]

#*/
