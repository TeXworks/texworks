# File to build unit-tests

include_directories("${CMAKE_SOURCE_DIR}/src" ${TeXworks_INCLUDE_DIRS})

# BiBTeXFile
add_executable(test_BibTeXFile BibTeXFile_test.cpp BibTeXFile_test.h "${CMAKE_SOURCE_DIR}/src/BibTeXFile.cpp" "${CMAKE_SOURCE_DIR}/src/BibTeXFile.h")
target_link_libraries(test_BibTeXFile ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_BibTeXFile COMMAND test_BibTeXFile WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# Scripting
add_executable(test_Scripting
	Scripting_test.cpp
	Scripting_test.h
	"${CMAKE_SOURCE_DIR}/src/scripting/JSScriptInterface.cpp"
	"${CMAKE_SOURCE_DIR}/src/scripting/Script.cpp"
	"${CMAKE_SOURCE_DIR}/src/scripting/JSScript.cpp"
	"${CMAKE_SOURCE_DIR}/src/Settings.cpp"
)
target_link_libraries(test_Scripting ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_Scripting COMMAND test_Scripting WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")

# LineNumberWidget
add_executable(test_LineNumberWidget
	LineNumberWidget_test.cpp
	LineNumberWidget_test.h
	"${CMAKE_SOURCE_DIR}/src/ui/LineNumberWidget.cpp"
)
target_link_libraries(test_LineNumberWidget ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_LineNumberWidget COMMAND test_LineNumberWidget WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")
