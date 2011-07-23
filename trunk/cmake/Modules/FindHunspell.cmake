# - Try to find Hunspell
# Once done this will define
#
#  HUNSPELL_FOUND - system has Hunspell
#  HUNSPELL_INCLUDE_DIR - the Hunspell include directory
#  HUNSPELL_LIBRARIES - Link these to use Hunspell
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( HUNSPELL_INCLUDE_DIR AND HUNSPELL_LIBRARIES )
   # in cache already
   SET(Hunspell_FIND_QUIETLY TRUE)
endif ( HUNSPELL_INCLUDE_DIR AND HUNSPELL_LIBRARIES )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)

  pkg_check_modules(HUNSPELL hunspell)
endif( NOT WIN32 )

FIND_PATH(HUNSPELL_INCLUDE_DIR NAMES hunspell.h
  PATHS
  /usr/include
  /usr/local/include
  ${HUNSPELL_INCLUDE_DIRS}
  PATH_SUFFIXES hunspell
)

FIND_LIBRARY(HUNSPELL_LIBRARIES NAMES hunspell
  PATHS
  /usr/lib
  /usr/local/lib
  ${HUNSPELL_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Hunspell DEFAULT_MSG HUNSPELL_INCLUDE_DIR HUNSPELL_LIBRARIES )

# show the HUNSPELL_INCLUDE_DIR and HUNSPELL_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(HUNSPELL_INCLUDE_DIR HUNSPELL_LIBRARIES )

