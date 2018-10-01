
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

add_library(SyncTeX STATIC ${SYNCTEX_SRCS} ${SYNCTEX_HDRS})
