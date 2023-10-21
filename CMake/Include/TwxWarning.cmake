#[===============================================[/*
This is part of the TWX build and test system.
https://github.com/TeXworks/texworks
(C)  JL 2023
*//** @file
Convenient shortcuts to manage warnings.

Usage:
```
include (TwxWarning)
```
*//**
Adapted to each compiler.
Values are reset to factory defaults
each time the script is included.
*/
TWX_WARNING_OPTIONS;
/*
#]===============================================]

# ANCHOR: TWX_WARNING_OPTIONS
#[=======[
# Initialize `TWX_WARNING_OPTIONS`
#]=======]
if ( NOT DEFINED TWX_WARNING_OPTIONS )
  if (MSVC)
    set (TWX_WARNING_OPTIONS /W4)
  else ()
    set (
      TWX_WARNING_OPTIONS
      -Wall -Wpedantic -Wextra -Wconversion
      -Wold-style-cast -Woverloaded-virtual
    )
  endif ()
endif ()

# ANCHOR: twx_warning_target
#[=======[
*//**
@brief Set warning options to the given target
@param target the name of an existing target
*/
twx_warning_target(target) {}
/*
#]=======]
function ( twx_warning_target target_ )
  target_compile_options (
    ${target_}
    PRIVATE ${TWX_WARNING_OPTIONS}
  )
endfunction ()

# ANCHOR: twx_warning_add
#[=======[
*//**
@brief Append the given warning options
@param warning some `-W...`
@param ... more `-W...`
*/
twx_warning_add ( warning, ... ) {}
/*
#]=======]
function ( twx_warning_add )
  list (
    APPEND
    TWX_WARNING_OPTIONS
    ${ARGN}
  )
endfunction ()
#*/
