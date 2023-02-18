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
	"${CMAKE_SOURCE_DIR}/src/scripting/ECMAScriptInterface.cpp"
	"${CMAKE_SOURCE_DIR}/src/scripting/ECMAScript.cpp"
	"${CMAKE_SOURCE_DIR}/src/Settings.cpp"
)
if (WITH_QTSCRIPT)
	target_sources(test_Scripting
		PRIVATE "${CMAKE_SOURCE_DIR}/src/scripting/JSScriptInterface.cpp"
		PRIVATE "${CMAKE_SOURCE_DIR}/src/scripting/JSScript.cpp"
	)
	target_compile_definitions(test_Scripting PRIVATE WITH_QTSCRIPT)
endif (WITH_QTSCRIPT)
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
	SignalCounter.cpp
	"${CMAKE_SOURCE_DIR}/src/ui/ClickableLabel.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/ClosableTabWidget.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/ColorButton.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/LineNumberWidget.cpp"
	"${CMAKE_SOURCE_DIR}/src/ui/ScreenCalibrationWidget.cpp"
)
target_compile_options(test_UI PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_UI ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_UI COMMAND test_UI WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# Utils
add_executable(test_Utils
	Utils_test.cpp
	Utils_test.h
	SignalCounter.cpp
	"${CMAKE_SOURCE_DIR}/src/utils/CommandlineParser.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/FileVersionDatabase.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/FullscreenManager.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/ResourcesLibrary.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/SystemCommand.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/TextCodecs.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/TypesetManager.cpp"
	"${CMAKE_SOURCE_DIR}/src/utils/VersionInfo.cpp"
)

target_compile_options(test_Utils PRIVATE ${WARNING_OPTIONS})
target_link_libraries(test_Utils ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_dependencies(test_Utils GitRev)
add_test(NAME test_Utils COMMAND test_Utils WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

if (APPLE)
	target_sources(test_Utils PRIVATE "${CMAKE_SOURCE_DIR}/src/utils/OSVersionInfo_mac.mm")
	target_link_libraries(test_Utils "-framework Foundation")
endif (APPLE)

# Document
add_executable(test_Document
	Document_test.cpp
	Document_test.h
	"${CMAKE_SOURCE_DIR}/src/document/Document.cpp"
	"${CMAKE_SOURCE_DIR}/src/document/SpellChecker.cpp"
	"${CMAKE_SOURCE_DIR}/src/document/TeXDocument.cpp"
	"${CMAKE_SOURCE_DIR}/src/document/TeXDocument.h"
	"${CMAKE_SOURCE_DIR}/src/document/TextDocument.cpp"
	"${CMAKE_SOURCE_DIR}/src/TWSynchronizer.cpp"
	"${CMAKE_SOURCE_DIR}/src/TWSynchronizer.h"
	"${CMAKE_SOURCE_DIR}/src/TeXHighlighter.h"
)
target_compile_options(test_Document PRIVATE ${WARNING_OPTIONS})
if (WITH_POPPLERQT)
	target_compile_definitions(test_Document PRIVATE WITH_POPPLERQT)
	if (POPPLER_HAS_RUNTIME_VERSION)
		target_compile_definitions(test_Document PRIVATE POPPLER_HAS_RUNTIME_VERSION)
	endif (POPPLER_HAS_RUNTIME_VERSION)
endif (WITH_POPPLERQT)
target_link_libraries(test_Document QtPDF::qtpdf SyncTeX::synctex Hunspell::hunspell ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_Document COMMAND test_Document WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

