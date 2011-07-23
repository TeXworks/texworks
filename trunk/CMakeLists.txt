# Support for certain parts of Qt, such as the Script Tools, was only added in
# v.2.8
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_COLOR_MAKEFILE ON)

IF ( NOT APPLE )
  MESSAGE( WARNING
    "This CMake build process has only been tested on Mac OS X."
    "\nIt may not work on other systems, but patches are welcome!"
  )
ENDIF ()

# Project name.
PROJECT(TeXworks)

# Canonical version number
SET(CPACK_PACKAGE_VERSION_MAJOR "0")
SET(CPACK_PACKAGE_VERSION_MINOR "5")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_PACKAGE_VERSION ${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})

# SVN revision
FILE(STRINGS ${PROJECT_SOURCE_DIR}/src/SvnRev.h TeXworks_SVN_REV LIMIT_COUNT 1)
STRING(REGEX MATCH "([0-9]+)" TeXworks_SVN_REV ${TeXworks_SVN_REV})
MESSAGE(STATUS "TeXworks SVN Revision: ${TeXworks_SVN_REV}")

# GIT revision---in case I upgrade the build script and make new binaries
# before the SVN revision advances. This could be removed if the CMake build
# process is ever imported into the main SVN repo.
EXECUTE_PROCESS(COMMAND git describe --always OUTPUT_VARIABLE TeXworks_GIT_REV OUTPUT_STRIP_TRAILING_WHITESPACE)
MESSAGE(STATUS "TeXworks Git Revision: ${TeXworks_GIT_REV}")


# Contains CMake scripts that help locate required components
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})

# State which parts of Qt4 are required.
SET(QT_USE_QTSCRIPT TRUE)
SET(QT_USE_QTSCRIPTTOOLS TRUE)
SET(QT_USE_QTXML TRUE)
SET(QT_USE_QTUITOOLS TRUE)

# Find required packages.
FIND_PACKAGE( Qt4 REQUIRED )
INCLUDE(${QT_USE_FILE})

FIND_PACKAGE(Fontconfig REQUIRED)
FIND_PACKAGE(Hunspell REQUIRED)
FIND_PACKAGE(Poppler REQUIRED)
FIND_PACKAGE(PopplerQT4 REQUIRED)
FIND_PACKAGE(ZLIB REQUIRED)

IF ( UNIX )
  IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/src/DefaultBinaryPaths.h )
    MESSAGE(STATUS "Generating DefaultBinaryPaths.h")

    EXECUTE_PROCESS(
      COMMAND ${TeXworks_SOURCE_DIR}/getDefaultBinPaths.sh
      WORKING_DIRECTORY ${TeXworks_SOURCE_DIR}
    )
  ENDIF ()
ENDIF ()

# Collect information on required include and library files.
SET(TeXworks_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${HUNSPELL_INCLUDE_DIR}
  ${POPPLERQT4_INCLUDE_DIR}
  ${FONTCONFIG_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

SET(TeXworks_LIB_DIRS
  ${QT_LIBRARY_DIRS}
  ${HUNSPELL_LIBRARY_DIRS}
  ${POPPLER_LIBRARY_DIRS}
  ${POPPLERQT4_LIBRARY_DIRS}
  ${FONTCONFIG_LIBRARY_DIRS}
)

SET(TeXworks_LIBS
  ${QT_LIBRARIES}
  ${POPPLER_LIBRARIES}
  ${POPPLERQT4_LIBRARIES}
  ${FONTCONFIG_LIBRARIES}
  ${HUNSPELL_LIBRARIES}
  ${ZLIB_LIBRARIES}
)


# Build TeXworks
ADD_SUBDIRECTORY(src)

# Post-build processing and packaging tasks
SET(CPACK_PACKAGE_NAME ${PROJECT_NAME})
SET(CPACK_PACKAGE_FILE_NAME "${CPACK_PACKAGE_NAME}.${CPACK_PACKAGE_VERSION}-${TeXworks_SVN_REV}-${TeXworks_GIT_REV}")
SET(CPACK_STRIP_FILES ON)

IF ( APPLE )
  # Nice way would be to put this in cmake/Modules with a descriptive filename
  # like MacBundler.cmake. However, it has to be brought in using
  # ADD_SUBDIRECTORY so that the INSTALL() directives are triggered after the
  # TeXworks `src` directory is processed. Otherwise, CMake tries to run
  # install_name_tool on a .app that hasn't been created yet.
  ADD_SUBDIRECTORY(${PROJECT_SOURCE_DIR}/cmake/packaging/mac)

  SET(CPACK_GENERATOR DragNDrop)
ENDIF ()

INCLUDE(CPack)

