# Variables used:
# - OUTPUTFILE (required)
# - PROJECT_NAME
# - Qt_LUPDATE_EXECUTABLE (optional)

set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR};${CMAKE_MODULE_PATH}")

include(TranslationMacros)

if (NOT OUTPUTFILE)
	message(FATAL_ERROR "No OUTPUTFILE specified")
endif (NOT OUTPUTFILE)

list(SORT FILES)
list(REMOVE_DUPLICATES FILES)

message(STATUS "Updating \"${OUTPUTFILE}\"")
create_qt_pro_file("${OUTPUTFILE}" INCLUDEPATH "${INCLUDEPATH}" FILES "${FILES}")

if (Qt_LUPDATE_EXECUTABLE)
	message(STATUS "Running lupdate")
	execute_process(COMMAND "${Qt_LUPDATE_EXECUTABLE}" "${OUTPUTFILE}")
endif (Qt_LUPDATE_EXECUTABLE)
