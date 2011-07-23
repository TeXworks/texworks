# - Try to find PopplerQT4
# Once done this will define
#
#  POPPLERQT4_FOUND - system has PopplerQT4
#  POPPLERQT4_INCLUDE_DIR - the PopplerQT4 include directory
#  POPPLERQT4_LIBRARIES - Link these to use PopplerQT4
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( POPPLERQT4_INCLUDE_DIR AND POPPLERQT4_LIBRARIES )
   # in cache already
   SET(PopplerQT4_FIND_QUIETLY TRUE)
endif ( POPPLERQT4_INCLUDE_DIR AND POPPLERQT4_LIBRARIES )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)

  pkg_check_modules(POPPLERQT4 poppler-qt4)
endif( NOT WIN32 )

FIND_PATH(POPPLERQT4_INCLUDE_DIR NAMES poppler-qt4.h poppler-link.h
  PATHS
  /usr/include
  /usr/local/include
  ${POPPLERQT4_INCLUDE_DIRS}
  PATH_SUFFIXES poppler qt4
)

FIND_LIBRARY(POPPLERQT4_LIBRARIES NAMES poppler-qt4
  PATHS
  /usr/lib
  /usr/local/lib
  ${POPPLERQT4_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(PopplerQT4 DEFAULT_MSG POPPLERQT4_INCLUDE_DIR POPPLERQT4_LIBRARIES )

# show the POPPLERQT4_INCLUDE_DIR and POPPLERQT4_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(POPPLERQT4_INCLUDE_DIR POPPLERQT4_LIBRARIES )

