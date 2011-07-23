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

PROJECT(TeXworks)

# Contains CMake scripts that help locate required components
SET(CMAKE_MODULE_PATH ${TeXworks_SOURCE_DIR}/cmake/Modules ${CMAKE_MODULE_PATH})

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

# Build TeXworks
ADD_SUBDIRECTORY(src)
