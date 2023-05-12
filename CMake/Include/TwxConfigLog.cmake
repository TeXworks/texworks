#[===============================================[/*
This is part of the TWX build and test system.
https://github.com/TeXworks/texworks
(C)  JL 2023
*//** @file
@brief Coloring log output of the summaries.

This is not available on windows.

Known formats:
  `BOLD`, `RED`, `GREEN`, `YELLOW`, `BLUE`, `MAGENTA`, `CYAN`, `WHITE`,
  `BOLD_RED`, `BOLD_GREEN`, `BOLD_YELLOW`, `BOLD_BLUE`, `BOLD_MAGENTA`, `BOLD_CYAN`, `BOLD_WHITE`
*/
/**
@brief Verbosity

When set more information is displayed.
*/
TWX_CONFIG_VERBOSE;
/**
@brief Coloring

Turn this off to disable coloring, or switch to windows.
*/
TWX_CONFIG_NO_COLOR;
/*
Output:

* `twx_log`
* `twx_config_log`
* `twx_config_log_kv`
* `twx_config_begin`
* `twx_config_end`

Each function is documented below.

#]===============================================]

if ( DEFINED twx-format-reset )
  return ()
endif ()

# Coloring output
# Standard feature to display colors on the terminal
if ( WIN32 OR TWX_CONFIG_NO_COLOR )
  set ( twx-format-reset )
  set ( twx-format-key )
  set ( twx-format-value )
else ()
  # One character to reset format
  string ( ASCII 27 TWX_TWENTY_SEVEN )
  set ( twx-format-reset "${TWX_TWENTY_SEVEN}[m" )
  # This is a poor man map
  set (
    twx-format
    BOLD         "1m"
    RED          "31m"
    GREEN        "32m"
    YELLOW       "33m"
    BLUE         "34m"
    MAGENTA      "35m"
    CYAN         "36m"
    WHITE        "37m"
    BOLD_RED     "1\;31m"
    BOLD_GREEN   "1\;32m"
    BOLD_YELLOW  "1\;33m"
    BOLD_BLUE    "1\;34m"
    BOLD_MAGENTA "1\;35m"
    BOLD_CYAN    "1\;36m"
    BOLD_WHITE   "1\;37m"
  )
endif ()

# ANCHOR: twx_log_format
#[=======[
*//**
@brief Formatter

Enclose the input between appropriate formatting characters,
put the result in the variable pointed to by output.

@param format is one of the known formats
@param output is the variable name holding the result

*/
twx_log_format( format output input ) {}
/*
#]=======]
function ( twx_log_format format output input )
  list ( FIND twx-format "${format}" _i )
  if    ( _i LESS 0 )
    set( ${output} "${input}" )
  else  ()
    math ( EXPR _i "${_i}+1" )
    list ( GET twx-format "${_i}" _l )
    set ( _l "${TWX_TWENTY_SEVEN}[${_l}" )
    set ( ${output} "${_l}${input}${twx-format-reset}" )
  endif ()
  twx_export ( ${output} )
endfunction ()

# ANCHOR: twx_log
#[=======[
*//**
@brief Print a message depending on a level.

@param format one of the known formats, optional
@param message some text
@param ... more messages
@param level is the log level, 0 to allways log, `+∞` to never log.
  `TWX_LOG_LEVEL_MAX` is the maximum value for display.
*/
twx_log ( [format] message ... [LEVEL level] ) {}
/**
@brief maximum value for display

Nothing is displayed if the given level is more than
`TWX_LOG_LEVEL_MAX`.
*/
TWX_LOG_LEVEL_MAX;
/*
#]=======]

if ( NOT DEFINED TWX_LOG_LEVEL_MAX )
  set ( TWX_LOG_LEVEL_MAX 0 )
endif ()

# `TWX_log_args_l` is a list variable.
# Empty `TWX_log_args_l` on `VERBOSE` mode.
# Parse the format, define shared variables
# `left` and `right`

# ANCHOR: __twx_config_log_parse
# Private macro to parse the leading `<format>`
# and the trailing `VERBOSE`.
macro ( __twx_config_log_parse )
# nothing to show if the whole section is hidden
  list( LENGTH TWX_log_args_l TWX_n )
  if ( TWX_n GREATER 0 )
    list ( GET TWX_log_args_l -1 TWX_l )
    set ( TWX_HIDE_l OFF )
    if ( TWX_l STREQUAL "VERBOSE" )
      list ( REMOVE_AT TWX_log_args_l -1 )
      if ( NOT TWX_CONFIG_VERBOSE )
        set ( TWX_HIDE_l ON )
      endif ()
    endif ()
    unset ( TWX_l )
    list( LENGTH TWX_log_args_l TWX_n )
    if ( TWX_n GREATER 0 )
      list ( GET TWX_log_args_l 0 TWX_format_l )
      list ( FIND twx-format "${TWX_format_l}" i )
      if ( i LESS 0 )
        set ( TWX_format_l )
      else ()
        list ( REMOVE_AT TWX_log_args_l 0 )
      endif ()
    endif ()
  endif ()
  unset ( TWX_n )
endmacro ()

# ANCHOR: twx_log
function ( twx_log )
set ( TWX_log_args_l ${ARGN} )
  __twx_config_log_parse ()
  if ( TWX_HIDE_l )
    # nothing to print
    message ( "" )
    return ()
  endif ()
  # Find the level
  cmake_parse_arguments ( MY "" "LEVEL" "" ${TWX_log_args_l} )
  if ( NOT DEFINED MY_LEVEL )
    set ( MY_LEVEL 0 )
  endif ()
  if ( NOT MY_LEVEL GREATER TWX_LOG_LEVEL_MAX )
    set ( msg "[TWX]:${MY_UNPARSED_ARGUMENTS}" )
    if ( TWX_format_l )
      twx_log_format ( "${TWX_format_l}" msg "${msg}" )
    endif ()
    message ( "${msg}" )
  endif ()
endfunction ()

option ( TWX_CONFIG_VERBOSE "Display more informations about the configuration" )
# NB: from the CLI use `cmake -DTWX_CONFIG_VERBOSE=ON ...`

# ANCHOR: twx_config_log
#[=======[ `twx_config_log`
*//**
@brief Basic logger

Other loggers depend on this one.

@param format is one of the knwon formats, optional
@param value optional text displayed on the right,
  with line break management
@param ... more optional values
@param VERBOSE optional key that disables logging when given,
  unless `TWX_CONFIG_VERBOSE` is set.
*/
twx_config_log( [format] message value ... [VERBOSE] ) {}
/*
#]=======]
function( twx_config_log )
  if ( TWX_CONFIG_section_hidden_l )
    return ()
  endif ()
  set ( TWX_log_args_l ${ARGN} )
    __twx_config_log_parse ()
  list( LENGTH TWX_log_args_l n )
  if ( TWX_HIDE_l OR NOT n GREATER 0 )
    message( "" )
    return ()
  endif ()
  list ( GET TWX_log_args_l 0 what )
  set ( what "${what}:" )
  list ( REMOVE_AT TWX_log_args_l 0 )
  list( LENGTH TWX_log_args_l n )
  if    ( NOT n GREATER 0 )
    if ( TWX_format_l )
      twx_log_format( ${TWX_format_l} what "${what}" )
    endif ()
    message ( "${TWX_config_indentation}${what}" )
    return ()
  endif ()
  # Hard wrap the remain material.
  string ( LENGTH "${what}" length_what )
  string ( LENGTH "${TWX_config_indentation}" length_indent )
  math ( EXPR left_char "30 - ${length_what} - ${length_indent}" )
  set ( blanks )
  foreach ( _i RANGE 1 ${left_char} )
    set ( blanks " ${blanks}" )
    # string( APPEND blanks " " ) in modern cMake
  endforeach ()
  # wrap the value to just more than 80 characters
  set ( _prefix "${TWX_config_indentation}${what}${blanks}" )
  # This is the prefix for the first line
  # for the next lines obtained by hard wrapping
  # this will be a blank string with the same length.
  string ( LENGTH "${_prefix}" _length )
  if ( TWX_format_l )
    twx_log_format( ${TWX_format_l} _prefix "${_prefix}" )
  endif ()
  set ( blanks )
  foreach ( _i RANGE 1 ${_length} )
    set ( blanks " ${blanks}" )
    # string( APPEND blanks " " ) in modern cMake
  endforeach()
  set ( _lines )
  foreach    ( item IN LISTS TWX_log_args_l )
    set ( _line "${_line} ${item}" )
    string ( LENGTH "${_line}" _length )
    if    ( _length GREATER 50 )
      if ( TWX_format_l )
        twx_log_format( ${TWX_format_l} _line "${_line}" )
      endif ()
      message ( "${_prefix}${_line}" )
      set ( _prefix "${blanks}" )
      # `what` and `_line` have been consumed, 
      set ( what )
      set ( _line )
    endif ()
  endforeach ()
  # Everything consumed?
  if    ( what OR _line )
    if ( TWX_format_l AND _line )
      twx_log_format( ${TWX_format_l} _line "${_line}" )
    endif ()
    message ( "${_prefix}${_line}" )
  endif ()
endfunction()

# ANCHOR: twx_config_log_kv
#[=======[
*//**
@brief .....key:....value lines

@param format one of the known formats, optional
@param key some label
@param value is displayed as `yes` or `no` with `FLAG`,
  variable content with `VAR` and as is otherwise.
@param VERBOSE mode, nothing is displayed except if
  `TWX_CONFIG_VERBOSE` is set.
*/
twx_config_log_kv ( [format] key [FLAG|VAR] value [VERBOSE] ) {}
/*
#]=======]
function( twx_config_log_kv )
  if ( TWX_CONFIG_section_hidden_l )
    return ()
  endif ()
  set ( TWX_log_args_l ${ARGN} )
  __twx_config_log_parse ()
  list( LENGTH TWX_log_args_l n )
  if ( TWX_HIDE_l OR NOT n GREATER 0 )
    message( "" )
    return ()
  endif ()
  list ( GET TWX_log_args_l 0 key )
  list ( REMOVE_AT TWX_log_args_l 0 )
  set ( value )
  list( LENGTH TWX_log_args_l n )
  if    ( NOT n GREATER 0 )
    set ( key "${key}:" )
    if ( TWX_format_l )
      twx_log_format( ${TWX_format_l} key "${key}" )
    endif ()
    message( "${TWX_config_indentation}${key}" )
    return ()
  endif ()
  list ( GET TWX_log_args_l 0 mode )
  list ( REMOVE_AT TWX_log_args_l 0 )
  if ( mode STREQUAL "FLAG" )
    list( LENGTH TWX_log_args_l n )
    if    ( NOT n GREATER 0 )
      set ( key "${key}:" )
      if ( TWX_format_l )
        twx_log_format( ${TWX_format_l} key "${key}" )
      endif ()
      message( "${TWX_config_indentation}${key}" )
      return ()
    endif ()  
    list ( GET TWX_log_args_l 0 value )
    list ( REMOVE_AT TWX_log_args_l 0 )
    if    ( ${value} )
      set ( value "yes" )
    else  ()
      set ( value "no" )
    endif ()
# Plz don't change yes/no to uppercase nor on/off
  elseif ( mode STREQUAL "VAR" )
    list( LENGTH TWX_log_args_l n )
    if    ( NOT n GREATER 0 )
      set ( key "${key}:" )
      if ( TWX_format_l )
        twx_log_format( ${TWX_format_l} key "${key}" )
      endif ()
      message( "${TWX_config_indentation}${key}" )
      return ()
    endif ()  
    list ( GET TWX_log_args_l 0 value )
    list ( REMOVE_AT TWX_log_args_l 0 )
    set ( value "${${value}}" )
  else ()
    set ( value "${mode}" )
  endif ()
  twx_config_log( ${TWX_format_l} "${key}" "${value}" "${TWX_log_args_l}" )
  endfunction()

# ANCHOR: twx_config_begin
#[=======[
*//**
@brief begin a new config section

Display the title and setup indentation.
Must be balanced by a `twx_config_end()`.

.
@param format optional known format
@param title required
@param VERBOSE optional. When `VERBOSE` is provided, the whole section is hidden
unless `TWX_CONFIG_VERBOSE` is set.
*/
twx_config_begin(format title VERBOSE) {}
/*
Implementation detail:
* `TWX_config_stack` keeps track of enclosing section.
  It is a list of `+` and `-`, the latter
  means that the section is hidden.
  **NB:** Testing that this list is empty is
  equivalent to testing for its content as string.
* `TWX_CONFIG_section_hidden_l` keeps track of
  the visibility state of the current section
* `TWX_config_indentation` is bigger in embedded sections.
#]=======]
set ( TWX_config_stack )
function ( twx_config_begin )
  set ( TWX_log_args_l ${ARGN} )
  # Is this section hidden?
  __twx_config_log_parse  ()
  if ( TWX_HIDE_l )
    set ( TWX_CONFIG_section_hidden_l on )
  endif ()
  if ( TWX_CONFIG_section_hidden_l )
    list ( INSERT TWX_config_stack 0 "-" )
  elseif ( TWX_config_stack )
    # Propagate the visibility state: duplicate and insert.
    list ( GET TWX_config_stack 0 _previous )
    list ( INSERT TWX_config_stack 0 "${_previous}" )
  else  ()
    list( INSERT TWX_config_stack 0 "+" )
  endif ()
  # export the main values
  set ( TWX_config_stack          "${TWX_config_stack}"          PARENT_SCOPE )
  set ( TWX_CONFIG_section_hidden_l "${TWX_CONFIG_section_hidden_l}" PARENT_SCOPE )
  if ( NOT TWX_CONFIG_section_hidden_l )
    set ( msg "${TWX_config_indentation}${TWX_log_args_l}" )
    if ( TWX_format_l )
      twx_log_format ( "${TWX_format_l}" msg "${msg}" )
    endif ()
    message ( "${msg}" )
  endif ()
  # build the indentation from scratch
  set ( TWX_config_indentation )
  foreach   ( _ IN LISTS TWX_config_stack )
    set ( TWX_config_indentation "  ${TWX_config_indentation}" )
  endforeach()
  set ( TWX_config_indentation "${TWX_config_indentation}" PARENT_SCOPE )
endfunction ()

# ANCHOR: twx_config_end
#[=======[
*//**
@brief Balance a `twx_config_begin`

End a config section, setup indentation and associate variables.
Must balance a previous `twx_config_begin` in the same scope.

@param NO_EOL optional key to remove an extra EOL
*/
twx_config_end( NO_EOL ) {}
/*
#]=======]
macro ( twx_config_end )
  set ( TWX_break_l ON )
  if    ( "${ARGN}" STREQUAL "NO_EOL" )
    set ( TWX_break_l OFF )
  endif ()
  if ( TWX_break_l AND NOT TWX_CONFIG_section_hidden_l )
    message( "" )
  endif ()
  set( TWX_config_indentation )
  if ( TWX_config_stack )
    list( REMOVE_AT TWX_config_stack 0 )
    if    ( TWX_config_stack )
      list( GET TWX_config_stack 0 TWX_l )
      if    ( "${TWX_l}" STREQUAL "-" )
        set( TWX_CONFIG_section_hidden_l ON )
      else  ()
        set ( TWX_CONFIG_section_hidden_l OFF )
      endif ()
      foreach   ( TWX_l IN LISTS TWX_config_stack )
        set ( TWX_config_indentation "  ${TWX_config_indentation}" )
      endforeach()    
      unset ( TWX_l )
    else  ()
      set ( TWX_CONFIG_section_hidden_l OFF )
    endif ()
    list ( LENGTH TWX_config_stack TWX_l )
    twx_log( ">>> HIDDEN: ${TWX_CONFIG_section_hidden_l}, DEPTH: ${TWX_l}" LEVEL 1000 )
    unset ( TWX_l )
  else ()
    twx_log( "Unexpected command `twx_config_end`.\n" )
  endif ()
  unset( TWX_break_l )
endmacro ()

#*/
