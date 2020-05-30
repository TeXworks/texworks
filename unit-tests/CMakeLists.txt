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


# UI
add_executable(test_UI
	UI_test.cpp
	UI_test.h
	"${CMAKE_SOURCE_DIR}/src/ui/ClickableLabel.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/ClosableTabWidget.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/LineNumberWidget.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/ScreenCalibrationWidget.cpp"
)
target_compile_options(test_UI PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_UI ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_UI COMMAND test_UI WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# FileVersionDatabase
add_executable(test_FileVersionDatabase
	FileVersionDatabase_test.cpp
	FileVersionDatabase_test.h
	"${CMAKE_SOURCE_DIR}/src/utils/FileVersionDatabase.cpp"
)
target_compile_options(test_FileVersionDatabase PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_FileVersionDatabase ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_FileVersionDatabase COMMAND test_FileVersionDatabase WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# Document
add_executable(test_Document
	Document_test.cpp
	Document_test.h
	"${CMAKE_SOURCE_DIR}/src/document/Document.cpp"
	"${CMAKE_SOURCE_DIR}/src/document/TeXDocument.cpp"
	"${CMAKE_SOURCE_DIR}/src/document/TeXDocument.h"
	"${CMAKE_SOURCE_DIR}/src/document/TextDocument.cpp"
	"${CMAKE_SOURCE_DIR}/src/TeXHighlighter.h"
)
target_compile_options(test_Document PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_Document ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_Document COMMAND test_Document WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

