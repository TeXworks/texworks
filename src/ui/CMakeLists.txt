#[===============================================[
This is part of TWX build and test system.
https://github.com/TeXworks/texworks

This is `<...>/src/ui/CMakeLists.txt`

It can be used as standalone testing project,
testing auxiliary or source provider.

## Standalone testing project

When used as primary `CMakeLists.txt` it defines
a standalone test suite for `Tw` namespace.

Usage from an external build directory:
```
cmake .../src/ui
cmake --build .
ctest .
```

## Subdirectory

When loaded by `add_subdirectory`.

### In testing context

It defines a test suite as part of a more general test.
See `/unit-tests/CMakeLists.txt`.

### In normal context

Output:
* `TwxUI_SOURCES`, a `;` separated list of full paths
* `TwxUI_HEADERS`, a `;` separated list of full paths

NB: The transition to Twx is not complete
#]===============================================]

if ( NOT TWX_IS_BASED )

	# This file is primary
	cmake_minimum_required ( VERSION 3.1 )
	include (
		"${CMAKE_CURRENT_LIST_DIR}/../../Cmake/Include/TwxBase.cmake"
		NO_POLICY_SCOPE
	)
	project ( TwxUI_Test )
	include ( TwxBase )

  set ( TWX_TEST ON	)
	set ( TwxUI_TEST_IS_ROOT ON )
	enable_testing ()
	include ( TwxQTLib )
	twx_fresh_Qt ()
	include ( TwxDoxydocLib )
  twx_doxydoc ( ${PROJECT_BINARY_DIR} )
elseif ( TWX_TEST )
	project ( TwxUI_Test )
	include ( TwxBase )
endif ()

include ( SrcTwxUISetup )

if ( NOT TWX_TEST )
	twx_export ( TwxUI_SOURCES )
	twx_export ( TwxUI_HEADERS )
	twx_export ( TwxUI_UIS )
	return ()
endif ()

list (
	APPEND
	TwxUI_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/Test/TwxUITest.cpp"
)
list (
	APPEND
	TwxUI_HEADERS
	"${CMAKE_CURRENT_LIST_DIR}/Test/TwxUITest.h"
)

# message ( STATUS "TwxUI_SOURCES ====> ${TwxUI_SOURCES}")
# message ( STATUS "TwxUI_UIS ====> ${TwxUI_UIS}")
# qt_wrap_ui (
# 	SOURCES
# 	ListSelectDialog.ui
# )
# message ( STATUS "SOURCES ====> ${SOURCES}")

add_executable (
	test_TwxUI
	${TwxUI_SOURCES} ${TwxUI_HEADERS} ${TwxUI_UIS}
)

twx_target_include_src ( test_TwxUI )

target_compile_definitions (
	test_TwxUI
	PRIVATE TwxUI_TEST
)

# if ( QT_VERSION_MAJOR EQUAL 6 )
# 	# Check for Qt6
# 	twx_append_QT ( REQUIRED Core5Compat )
# else ()
# 	if ( WIN32 AND NOT BUILD_SHARED_LIBS )
# 		twx_append_QT ( REQUIRED WindowsPlatformSupport )
# 	endif ()
# endif ()

twx_append_QT ( REQUIRED Widgets )

target_link_libraries (
	test_TwxUI
	${QT_LIBRARIES}
)

include ( TwxWarning )
twx_warning_target ( test_TwxUI )

include ( TwxTestCase )
twx_test_case ( test_TwxUI tws_working_directory )

add_test (
	NAME test_TwxUI
	COMMAND test_TwxUI
	WORKING_DIRECTORY
		"${tws_working_directory}"
)

add_executable(
	byte_echo_TwxUI
	${CMAKE_CURRENT_LIST_DIR}/Test/byte_echo_test.cpp
)
twx_warning_target ( byte_echo_TwxUI )

set_target_properties (
	byte_echo_TwxUI
  PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY "${tws_working_directory}"
		SUFFIX ".program"
)

unset ( tws_working_directory )

# ANCHOR - Summary
# =======

if ( TwxUI_TEST_IS_ROOT )

	message ( "" )

	include ( TwxConfigLog )

	twx_config_begin (
		BOLD_GREEN
	"${PROJECT_NAME} test suite has been configured \
	(CMake ${CMAKE_VERSION}, \
	${CMAKE_CXX_COMPILER_ID} \
	${CMAKE_CXX_COMPILER_VERSION}):\n"
		)
	
	twx_config_begin ( BOLD_MAGENTA "Version info" )
	twx_config_log ( "Qt" ${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH} )
	twx_config_end ()

	twx_config_log ( "Compiler optimization:" ${CMAKE_BUILD_TYPE} )
	twx_config_log ( "" )

	twx_config_begin ( BOLD_BLUE "Libraries" VERBOSE )
	twx_config_log_kv ( "Qt${QT_VERSION_MAJOR}" VAR QT_LIBRARIES )
	twx_config_end ()

	twx_config_begin ( BOLD_BLUE "Build settings" VERBOSE )
	get_target_property (
		COMPILE_DEFINITIONS
		test_TwxUI
		COMPILE_DEFINITIONS
	)
	twx_config_log_kv ( "Compile definitions" VAR COMPILE_DEFINITIONS )
	get_target_property (
		COMPILE_OPTIONS
		test_TwxUI
		COMPILE_OPTIONS
	)
	twx_config_log_kv ( "Compile options" VAR COMPILE_OPTIONS )
	twx_config_end ()

	twx_config_begin ( BOLD_BLUE "Files" VERBOSE )
	twx_config_log_kv ( "SOURCES" VAR TwxUI_SOURCES )
	twx_config_log_kv ( "HEADERS" VAR TwxUI_HEADERS )
	twx_config_log_kv ( "UIS"			VAR TwxUI_UIS )
	twx_config_end ()

	twx_config_end ( NO_EOL )

endif ()
