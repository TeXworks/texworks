include_directories("${CMAKE_SOURCE_DIR}/src" ${TeXworks_INCLUDE_DIRS})

add_executable(test_BibTeXFile BibTeXFile_test.cpp BibTeXFile_test.h "${CMAKE_SOURCE_DIR}/src/BibTeXFile.cpp" "${CMAKE_SOURCE_DIR}/src/BibTeXFile.h")
target_link_libraries(test_BibTeXFile ${QT_LIBRARIES} ${ZLIB_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
add_test(NAME test_BibTeXFile COMMAND test_BibTeXFile WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/testcases")
