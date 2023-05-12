#[===============================================[/*
This is part of the TWX build and test system.
See https://github.com/TeXworks/texworks
(C)  JL 2023
*//** @file
@brief  Doxygen support

Doxygen support to generate source documentation.
See @ref CMake/README.md.

Usage (`TwxBase` is required) :
```
  include ( TwxDoxydocLib )
  twx_doxydoc (...)
```
Output:

- `twx_doxydoc()`

*/
/*#]===============================================]

if ( NOT TWX_IS_BASED )
  message ( FATAL_ERROR "Missing `TwxBase`" )
  return ()
endif ()

if ( COMMAND twx_doxydoc )
  return ()
endif ()

find_package ( Doxygen )

if ( NOT DOXYGEN_FOUND )
  function ( twx_doxydoc )
    message (
      STATUS
      "Install Doxygen to generate the developer documentation"
    )
  endfunction ()
  return ()
endif ()

option (
  TWX_DOCUMENT_TEST_SUITES
  "Document the Test suites"
  OFF
)

# ANCHOR: twx_doxydoc
#[=======[*/
/*! @fn twx_doxydoc(binary_dir)

Generate source documentation with a target.

Put `twx_doxydoc(binary_dir)` in the main `CMakeLists.txt`
and run `make doxydoc` from the command line in that same build directory.
The documentation is then available at `<binary_dir>/doxydoc/`.

This function is one shot. Next invocation will issue a warning.
If Doxygen is not installed, this function is a noop.

Input:
- `.../Developer/doxydoc.in.txt` is the configuration file

    @param binary_dir a path.
 */
void twx_doxydoc(binary_dir) {}
/*#]=======]
function ( twx_doxydoc BINARY_DIR )
  twx_assert_non_void ( TWX_DIR )
  if ( TARGET doxydoc )
    message ( WARNING "doxydoc target already defined" )
    return ()
  endif ()
  # set input and output files
  set (
    twx_in
    "${TWX_DIR}/Developer/doxydoc.in.txt"
  )
  set (
    twx_out
    "${BINARY_DIR}/build_data/doxydoc.txt"
  )
  set (
    TWX_CFG_DOXYGEN_OUTPUT_DIRECTORY
    ${BINARY_DIR}/doxydoc/
  )
  if ( TWX_DOCUMENT_TEST_SUITES )
    set ( TWX_CFG_DOXYGEN_EXCLUDE )
  else ()
    set ( TWX_CFG_DOXYGEN_EXCLUDE */Test/* )
  endif ()
  configure_file ( ${twx_in} ${twx_out} @ONLY )
  add_custom_target(
    doxydoc
    COMMAND ${DOXYGEN_EXECUTABLE} ${twx_out}
    WORKING_DIRECTORY ${TWX_DIR}
    COMMENT "Generating developer documentation with Doxygen"
    VERBATIM
  )
endfunction ( twx_doxydoc )

#[=======[
*/
#]=======]
