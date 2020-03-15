# File to build unit-tests

include_directories("${CMAKE_SOURCE_DIR}/src" ${TeXworks_INCLUDE_DIRS})

# BiBTeXFile
add_executable(test_BibTeXFile BibTeXFile_test.cpp BibTeXFile_test.h "${CMAKE_SOURCE_DIR}/src/BibTeXFile.cpp" "${CMAKE_SOURCE_DIR}/src/BibTeXFile.h")
target_compile_options(test_BibTeXFile PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_BibTeXFile ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_BibTeXFile COMMAND test_BibTeXFile WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# Scripting
add_executable(test_Scripting
	Scripting_test.cpp
	Scripting_test.h
	MockScriptingAPI.h
	"${CMAKE_SOURCE_DIR}/src/scripting/Script.cpp"
	"${CMAKE_SOURCE_DIR}/src/scripting/JSScriptInterface.cpp"
	"${CMAKE_SOURCE_DIR}/src/scripting/JSScript.cpp"
	"${CMAKE_SOURCE_DIR}/src/Settings.cpp"
)
target_compile_options(test_Scripting PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_Scripting ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_Scripting COMMAND test_Scripting WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

IF ( LUA_FOUND AND WITH_LUA )
	add_executable(test_LuaScripting
		LuaScripting_test.cpp
		LuaScripting_test.h
		MockScriptingAPI.h
		"${CMAKE_SOURCE_DIR}/src/scripting/Script.cpp"
		"${CMAKE_SOURCE_DIR}/src/Settings.cpp"
	)
	target_compile_options(test_LuaScripting PRIVATE ${WARNING_OPTIONS})
	IF (NOT ${BUILD_SHARED_PLUGINS})
		target_link_libraries(test_LuaScripting TWLuaPlugin)
	ENDIF()

	target_link_libraries(test_LuaScripting ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
	add_test(NAME test_LuaScripting COMMAND test_LuaScripting WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")
ENDIF ()


# LineNumberWidget
add_executable(test_LineNumberWidget
	LineNumberWidget_test.cpp
	LineNumberWidget_test.h
	"${CMAKE_SOURCE_DIR}/src/ui/LineNumberWidget.cpp"
)
target_compile_options(test_LineNumberWidget PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_LineNumberWidget ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_LineNumberWidget COMMAND test_LineNumberWidget WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# FileVersionDatabase
add_executable(test_FileVersionDatabase
	FileVersionDatabase_test.cpp
	FileVersionDatabase_test.h
	"${CMAKE_SOURCE_DIR}/src/utils/FileVersionDatabase.cpp"
)
target_compile_options(test_FileVersionDatabase PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_FileVersionDatabase ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_FileVersionDatabase COMMAND test_FileVersionDatabase WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

