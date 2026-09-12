
# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)

  pkg_check_modules(ICU_PKG QUIET icu-uc)
endif( NOT WIN32 )

find_library(ICU_LIBRARY NAMES icuuc HINTS ${ICU_PKG_LIBRARY_DIRS})
find_library(ICU_DATA NAMES icudata icudt HINTS ${ICU_PKG_LIBRARY_DIRS})
find_path(ICU_INCLUDE_DIR unicode/ucnv.h HINTS ${ICU_PKG_INCLUDE_DIRS})

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(ICU DEFAULT_MSG ICU_LIBRARY ICU_DATA ICU_INCLUDE_DIR)
mark_as_advanced(ICU_LIBRARY ICU_DATA ICU_INCLUDE_DIR)

if (ICU_FOUND)
	if (NOT TARGET ICU::uc)
		add_library(ICU::uc UNKNOWN IMPORTED)
		set_target_properties(ICU::uc PROPERTIES IMPORTED_LOCATION ${ICU_LIBRARY} INTERFACE_INCLUDE_DIRECTORIES ${ICU_INCLUDE_DIR})
	endif ()
	if (NOT TARGET ICU::data)
		add_library(ICU::data UNKNOWN IMPORTED)
		set_target_properties(ICU::data PROPERTIES IMPORTED_LOCATION ${ICU_DATA})
	endif ()
endif ()
