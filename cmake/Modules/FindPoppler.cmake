# - Try to find Poppler
# Once done this will define
#
#  POPPLER_FOUND - system has Poppler
#  POPPLER_XPDF_INCLUDE_DIR - the include directory for Poppler XPDF headers
#  POPPLER_LIBRARIES - Link these to use Poppler
# Note: the Poppler include directory is currently not needed by TeXworks
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.
#


if ( POPPLER_LIBRARIES )
   # in cache already
   SET(Poppler_FIND_QUIETLY TRUE)
endif ( POPPLER_LIBRARIES )

# use pkg-config to get the directories and then use these values
# in the FIND_PATH() and FIND_LIBRARY() calls
if( NOT WIN32 )
  find_package(PkgConfig)

  pkg_check_modules(POPPLER poppler)
endif( NOT WIN32 )

# Check for Poppler XPDF headers
FIND_PATH(POPPLER_XPDF_INCLUDE_DIR NAMES poppler-config.h
  PATHS
  /usr/include
  /usr/local/include
  ${POPPLER_INCLUDE_DIRS}
  PATH_SUFFIXES poppler
)

IF( NOT(POPPLER_XPDF_INCLUDE_DIR) )

  MESSAGE( STATUS "Could not find poppler-config.h, disabling support for Xpdf headers." )

  SET( POPPLER_HAS_XPDF false )

ELSE( NOT(POPPLER_XPDF_INCLUDE_DIR) )

  SET( POPPLER_HAS_XPDF true )

ENDIF( NOT(POPPLER_XPDF_INCLUDE_DIR) )

FIND_LIBRARY(POPPLER_LIBRARIES NAMES poppler
  PATHS
  /usr/lib
  /usr/local/lib
  ${POPPLER_LIBRARY_DIRS}
)

include(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(Poppler DEFAULT_MSG POPPLER_LIBRARIES )

# show the POPPLER_(XPDF)INCLUDE_DIR and POPPLER_LIBRARIES variables only in the advanced view
MARK_AS_ADVANCED(POPPLER_XPDF_INCLUDE_DIR POPPLER_LIBRARIES)

