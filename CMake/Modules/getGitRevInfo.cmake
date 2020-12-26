find_package(Git QUIET)

if (NOT OUTPUT_DIR)
	message(FATAL_ERROR "OUTPUT_DIR not set")
endif (NOT OUTPUT_DIR)

set(OLD_HASH "")
set(OLD_DATE "")
set(SUCCESS FALSE)

# Recover old git commit info from `GitRev.h` if available.
# We don't want to touch the file if nothing relevant has changed as that would
# trigger an unnecessary rebuild of parts of the project
if (EXISTS "${OUTPUT_DIR}/GitRev.h")
	file(STRINGS ${OUTPUT_DIR}/GitRev.h GIT_INFO REGEX "#define GIT_COMMIT_HASH \"([a-f0-9]+\\*?)\"")
	string(REGEX REPLACE "#define GIT_COMMIT_HASH \"([a-f0-9]+\\*?)\"" "\\1" OLD_HASH "${GIT_INFO}")
	file(STRINGS ${OUTPUT_DIR}/GitRev.h GIT_INFO REGEX "#define GIT_COMMIT_DATE \"([-+:0-9 Z]+)\"")
	string(REGEX REPLACE "#define GIT_COMMIT_DATE \"([-+:0-9 Z]+)\"" "\\1" OLD_DATE "${GIT_INFO}")
endif()

# Try to run git to obtain the last commit hash and date
if (GIT_FOUND)
	execute_process(COMMAND "${GIT_EXECUTABLE}" "--git-dir=.git" "show" "--no-patch" "--pretty=%h" RESULT_VARIABLE HASH_RESULT OUTPUT_VARIABLE NEW_HASH OUTPUT_STRIP_TRAILING_WHITESPACE)
	execute_process(COMMAND "${GIT_EXECUTABLE}" "--git-dir=.git" "show" "--no-patch" "--pretty=%ci" RESULT_VARIABLE DATE_RESULT OUTPUT_VARIABLE NEW_DATE OUTPUT_STRIP_TRAILING_WHITESPACE)

	if (${HASH_RESULT} EQUAL 0 AND ${DATE_RESULT} EQUAL 0 AND NOT "${NEW_HASH}" STREQUAL "" AND NOT "${NEW_DATE}" STREQUAL "")
		set(SUCCESS TRUE)
		execute_process(COMMAND "${GIT_EXECUTABLE}" "--git-dir=.git" "diff" "--ignore-cr-at-eol" "--quiet" "HEAD" RESULT_VARIABLE MODIFIED_RESULT)
		if ("${MODIFIED_RESULT}" EQUAL 1)
			set(NEW_HASH "${NEW_HASH}*")
		endif ("${MODIFIED_RESULT}" EQUAL 1)
	endif (${HASH_RESULT} EQUAL 0 AND ${DATE_RESULT} EQUAL 0 AND NOT "${NEW_HASH}" STREQUAL "" AND NOT "${NEW_DATE}" STREQUAL "")
endif (GIT_FOUND)

if (NOT SUCCESS)
	# Maybe this is an exported source code and not a git clone
	# Try to retrieve the export commit hash and date from GitArchiveInfo.txt
	file(STRINGS GitArchiveInfo.txt GIT_INFO REGEX "#define GIT_COMMIT_HASH \"([a-f0-9]+)\"")
	string(REGEX REPLACE "#define GIT_COMMIT_HASH \"([a-f0-9]+)\"" "\\1" NEW_HASH "${GIT_INFO}")
	file(STRINGS GitArchiveInfo.txt GIT_INFO REGEX "#define GIT_COMMIT_DATE \"([-+:0-9 Z]+)\"")
	string(REGEX REPLACE "#define GIT_COMMIT_DATE \"([-+:0-9 Z]+)\"" "\\1" NEW_DATE "${GIT_INFO}")

	if (NOT "${NEW_HASH}" STREQUAL "" AND NOT "${NEW_DATE}" STREQUAL "")
		set(SUCCESS TRUE)
	endif (NOT "${NEW_HASH}" STREQUAL "" AND NOT "${NEW_DATE}" STREQUAL "")
endif (NOT SUCCESS)

if (SUCCESS)
	if (NOT "${OLD_HASH}" STREQUAL "${NEW_HASH}" OR NOT "${OLD_DATE}" STREQUAL "${NEW_DATE}")
		# If everything worked and the data has changed, update the output file
		file (WRITE "${OUTPUT_DIR}/GitRev.h" "// This file is used to identify the latest git commit. Please do not touch.\n#define GIT_COMMIT_HASH \"${NEW_HASH}\"\n#define GIT_COMMIT_DATE \"${NEW_DATE}\"\n")
		message(STATUS "Git commit info updated")
	endif (NOT "${OLD_HASH}" STREQUAL "${NEW_HASH}" OR NOT "${OLD_DATE}" STREQUAL "${NEW_DATE}")
else (SUCCESS)
	if ("${OLD_HASH}" STREQUAL "" OR "${OLD_DATE}" STREQUAL "")
		message(FATAL_ERROR "Could not determine git commit info")
	else ("${OLD_HASH}" STREQUAL "" OR "${OLD_DATE}" STREQUAL "")
		message(WARNING "Could not determine git commit info")
	endif ("${OLD_HASH}" STREQUAL "" OR "${OLD_DATE}" STREQUAL "")
endif (SUCCESS)

