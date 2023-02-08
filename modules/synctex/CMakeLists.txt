# File to build <a href="https://github.com/jlaurens/synctex/">SyncTeX</a> by
# Jérôme Laurens into a static library

set(SYNCTEX_SRCS
  synctex_parser.c
  synctex_parser_utils.c
)
set(SYNCTEX_HDRS
  synctex_parser.h
  synctex_parser_advanced.h
  synctex_parser_utils.h
  synctex_version.h
)

find_package(ZLIB REQUIRED)

add_library(SyncTeX STATIC ${SYNCTEX_SRCS} ${SYNCTEX_HDRS})

set_target_properties(SyncTeX PROPERTIES
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(SyncTeX ZLIB::ZLIB)
if (WIN32)
	target_link_libraries(SyncTeX shlwapi)
endif()
