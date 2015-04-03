# Setup and User Options
# ======================

# Set project name.
PROJECT(QtPDF)

# We require CMake v2.8 or greater because we don't do any testing with earlier
# versions.
# In CMake v2.8.6, the AUTOMOC feature was introduced which simplifies the
# interaction with Qt.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8.6)
SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_AUTOMOC TRUE)

# Always add the current source and binary directories to the header include
# path when compiling.
SET(CMAKE_INCLUDE_CURRENT_DIR ON)


# Determine Version Numbers
# -------------------------
set(PROJECT_VERSION "0.1")


# Declare Project Options
# -----------------------

# For now, default to a debug build.
IF ( NOT CMAKE_BUILD_TYPE )
  SET(CMAKE_BUILD_TYPE "Debug")
ENDIF ()

# By default, we build a shared lib...
IF ( NOT DEFINED BUILD_SHARED_LIBS )
  SET(BUILD_SHARED_LIBS ON)
ENDIF ()

# ...with poppler-qt...
IF ( NOT DEFINED WITH_POPPLERQT )
  SET(WITH_POPPLERQT ON)
ENDIF ()

# ...but without MuPDF
IF ( NOT DEFINED WITH_MUPDF )
  SET(WITH_MUPDF OFF)
ENDIF ()

# ...and the viewer programs
IF ( NOT DEFINED QTPDF_VIEWER )
  SET(QTPDF_VIEWER ON)
ENDIF ()

OPTION(QTPDF_VIEWER "Build PDF viewer application" ${QTPDF_VIEWER})

OPTION(WITH_POPPLER "Build Poppler Qt backend" ${WITH_POPPLERQT})
# MuPDF backend is a bit immature, so we don't bother with it by default right
# now.
OPTION(WITH_MUPDF "Build MuPDF backend" ${WITH_MUPDF})


OPTION(BUILD_SHARED_LIBS "Build shared library" ${BUILD_SHARED_LIBS})

# Dependency Configuration
# ========================

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as Poppler.
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules)


# Core Components
# ---------------

set(DESIRED_QT_VERSION "" CACHE STRING "Pick a version of Qt to use: 4 or 5")
MARK_AS_ADVANCED(DESIRED_QT_VERSION)

# Check for Qt4 by default
IF (NOT DESIRED_QT_VERSION OR DESIRED_QT_VERSION MATCHES 4)
  # Declare Qt libraries required by this project.
  SET (QT_COMPONENTS
    QtCore
    QtGui
    QtXML
  )
  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    LIST(APPEND QT_COMPONENTS QtTest)
  ENDIF()

  # Locate Qt. `INCLUDE(UseQt4)` brings in a set of macros that helps us deal
  # with Qt-specific tasks such as compiling resources or running `moc`.
  #
  # Qt 4.6.0 or newer is required for many pieces of functionality used in the
  # code.
  FIND_PACKAGE(Qt4 4.6.0 COMPONENTS ${QT_COMPONENTS} QUIET)
  INCLUDE(UseQt4)
  
  # TODO: QtTest is included in the list of "normal" libraries for debug builds.
  # Therefore, QT_TEST_LIBRARIES is not set here. Can we improve this somehow
  # (i.e., can we pull in QtTest only for programs that actually use it)?

  # Define version-dependent wrappers
  MACRO(QT_ADD_RESOURCES)
    QT4_ADD_RESOURCES(${ARGV})
  ENDMACRO(QT_ADD_RESOURCES)
ENDIF()
# If Qt4 is not found (or not desired), check for Qt5
IF (DESIRED_QT_VERSION MATCHES 5 OR NOT QT4_FOUND)
  find_package(Qt5Widgets QUIET)
  find_package(Qt5Concurrent QUIET)
  find_package(Qt5Xml QUIET)

  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    find_package(Qt5Test QUIET)
  ENDIF()
  
  IF( WIN32 AND NOT BUILD_SHARED_LIBS )
    # For static Windows builds, we also need to pull in the Qt5 Platform
    # support library, which is not exposed to CMake properly, unfortunately
    GET_TARGET_PROPERTY(QT_LIB_DIR "${Qt5Widgets_LIBRARIES}" LOCATION)
    GET_FILENAME_COMPONENT(QT_LIB_DIR "${QT_LIB_DIR}" PATH)
    FIND_LIBRARY(Qt5Platform_LIBRARIES Qt5PlatformSupport
      HINTS "${QT_LIB_DIR}"
    )
    FIND_LIBRARY(Qt5QWindows_LIBRARIES qwindows
      HINTS "${QT_LIB_DIR}/../plugins/platforms"
    )
    ADD_DEFINITIONS(-DSTATIC_QT5)
  ENDIF()

  IF(Qt5Widgets_FOUND AND Qt5Concurrent_FOUND AND Qt5Core_FOUND AND (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Debug" OR (Qt5Test_FOUND AND Qt5Xml_FOUND)))
  	SET(QT5_FOUND TRUE)
    SET(QT_INCLUDE_DIR "${Qt5Widgets_INCLUDE_DIRS} ${Qt5Concurrent_INCLUDE_DIRS}")
    SET(QT_LIBRARIES ${Qt5QWindows_LIBRARIES} ${Qt5Platform_LIBRARIES} ${Qt5Widgets_LIBRARIES} ${Qt5Concurrent_LIBRARIES} ${Qt5Core_LIBRARIES} ${Qt5Xml_LIBRARIES})
    SET(QT_TEST_LIBRARIES ${Qt5Test_LIBRARIES})

    # Note: Qt5 only sets Qt5Widgets_VERSION, etc., but not QT_VERSION_MAJOR,
    # etc. which is used here.
    string(REGEX REPLACE "^([0-9]+).*$" "\\1" QT_VERSION_MAJOR "${Qt5Widgets_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_MINOR  "${Qt5Widgets_VERSION}")
    string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_PATCH "${Qt5Widgets_VERSION}")
  ENDIF()
  
  # Define version-dependent wrappers
  MACRO(QT_ADD_RESOURCES)
    QT5_ADD_RESOURCES(${ARGV})
  ENDMACRO(QT_ADD_RESOURCES)
ENDIF()

IF (NOT QT4_FOUND AND NOT QT5_FOUND)
  MESSAGE(FATAL_ERROR "Could not find required Qt >= 4.6. Please install Qt (see http://qt-project.org/) and/or adjust DESIRED_QT_VERSION.")
ENDIF()

# Expose the major version number of Qt to the preprocessor. This is necessary
# to include the correct Qt headers (as QTVERSION is not defined before any Qt
# headers are included)
ADD_DEFINITIONS(-DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})


FIND_PACKAGE(ZLIB REQUIRED)

# Aggregate library names and include directories into variables for easy
# access.
SET(QTPDF_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
  ${POPPLER_QT_INCLUDE_DIR}
)

SET(QTPDF_LIBS
  ${QT_LIBRARIES}
  ${ZLIB_LIBRARIES}
)


# Backend Components
# ------------------
IF( WITH_POPPLERQT )

  FIND_PACKAGE(Poppler REQUIRED)
  LIST(APPEND QTPDF_INCLUDE_DIRS ${POPPLER_QT_INCLUDE_DIR})
  # In case poppler is linked statically, its libs have to be given to the
  # linker **first** for some strange reason or symbols won't be properly
  # resolved. At least, this is the case with the MinGW linker.
  SET(QTPDF_LIBS ${POPPLER_LIBRARIES} ${QTPDF_LIBS})

  # The only thing Poppler should need is the location of the include
  # directories in order to access header files. The library loader should be
  # able to find libfontconfig on standard search paths.
  IF ( POPPLER_NEEDS_FONTCONFIG )
    FIND_PACKAGE(Fontconfig REQUIRED)
    LIST(APPEND QTPDF_INCLUDE_DIRS ${FONTCONFIG_INCLUDE_DIR})
  ENDIF ()

ENDIF()

IF( WITH_MUPDF )

  FIND_PACKAGE(JPEG REQUIRED)
  FIND_PACKAGE(Freetype REQUIRED)
  FIND_PACKAGE(JBig2Dec REQUIRED)
  FIND_PACKAGE(OpenJPEG REQUIRED)
  FIND_PACKAGE(MuPDF REQUIRED)

  LIST(APPEND QTPDF_INCLUDE_DIRS
    ${JPEG_INCLUDE_DIR}
    ${FREETYPE_INCLUDE_DIR}
    ${JBIG2DEC_INCLUDE_DIR}
    ${OPENJPEG_INCLUDE_DIR}
    ${MUPDF_INCLUDE_DIR}
  )

  LIST(APPEND QTPDF_LIBS
    ${JPEG_LIBRARIES}
    ${FREETYPE_LIBRARIES}
    ${JBIG2DEC_LIBRARIES}
    ${OPENJPEG_LIBRARIES}
  )

  # Since the MuPDF libraries are static libs, they have to be given to the
  # linker **first** for some strange reason or symbols won't be properly
  # resolved. At least, this is the case with the MinGW linker.
  SET(QTPDF_LIBS ${MUPDF_LIBRARIES} ${QTPDF_LIBS})

  # setlocale() is necessary for the MuPDF backend (at least for locales not
  # using '.' as a decimal point)
  INCLUDE( CheckIncludeFiles )
  INCLUDE( CheckFunctionExists )
  CHECK_INCLUDE_FILES(locale.h HAVE_LOCALE_H)
  CHECK_FUNCTION_EXISTS(setlocale HAVE_SETLOCALE)
  IF( HAVE_LOCALE_H AND HAVE_SETLOCALE )
    ADD_DEFINITIONS(-DHAVE_LOCALE_H)
  ENDIF()

ENDIF()


# Update Header Templates
# -----------------------


# Building
# ========

# Common setup.

INCLUDE_DIRECTORIES(
  ${QTPDF_INCLUDE_DIRS}
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
)

IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
  ADD_DEFINITIONS(-DDEBUG -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)
  SET(CMAKE_CXX_FLAGS_DEBUG "-g -Wall")
ENDIF()


# Library
# -------
SET(QTPDF_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentWidget.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentTools.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFTransitions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFActions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFAnnotations.cpp
)

SET(QTPDF_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentTools.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFTransitions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFActions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFAnnotations.h
)

# FIXME: Is -fPIC required/appropriate for all situations/platforms?
IF( NOT WIN32 )
  SET(QTPDF_FLAGS "-fPIC")
ENDIF()

IF( WITH_POPPLERQT )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQtBackend.cpp)
  LIST(APPEND QTPDF_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQtBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_POPPLERQT")
ENDIF()

IF( WITH_MUPDF )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.cpp)
  LIST(APPEND QTPDF_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_MUPDF")
ENDIF()

# Create translations
INCLUDE( TranslationMacros )
FILE(GLOB TRANSLATIONS_SOURCES trans/*.ts)
FILE(GLOB TRANSLATIONS_QM trans/*.qm)

QT_ADD_QM_TRANSLATIONS(QTPDF_QM ${TRANSLATIONS_QM})

CREATE_QT_PRO_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.pro" "src" ${QTPDF_SRCS} ${QTPDF_HDRS} ${TRANSLATIONS_SOURCES})

# Icons
SET(QTPDF_RCS
  ${CMAKE_CURRENT_SOURCE_DIR}/QtPDF_icons.qrc
)
QT_ADD_RESOURCES(QTPDF_RESOURCES ${QTPDF_RCS})


ADD_LIBRARY(qtpdf
  ${QTPDF_SRCS}
  ${QTPDF_RESOURCES}
  ${QTPDF_QM}
)

set(QTPDF_ADDITIONAL_LIBS "" CACHE STRING "Additional libraries not found by CMake")
MARK_AS_ADVANCED(QTPDF_ADDITIONAL_LIBS)

TARGET_LINK_LIBRARIES(qtpdf ${QTPDF_LIBS} ${QTPDF_ADDITIONAL_LIBS})

SET_TARGET_PROPERTIES(qtpdf PROPERTIES
  COMPILE_FLAGS "${QTPDF_FLAGS}"
)

# Inform parent scope (if any) what to do to use QtPDF
SET(QTPDF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/backend


# Viewers
# -------

IF ( QTPDF_VIEWER )

# Both viewers use a common set of source code files. Preprocessor definitions
# toggle the backend-sensitive bits.
SET(PDFVIEWER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PDFViewer.cpp
)

QT_ADD_RESOURCES(PDFVIEWER_RESOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/icons.qrc
)

SET(QTPDFTEST_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/TestQtPDF.cpp
)



IF( WITH_POPPLERQT )
  ADD_EXECUTABLE(poppler-qt${QT_VERSION_MAJOR}_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_RESOURCES}
  )

  # NOTE:
  # Setting properties on a target sets those properties on all source files
  # that are built to create that target. Other targets can re-use the sources
  # and build with different flags. Pretty handy.
  SET_TARGET_PROPERTIES(poppler-qt${QT_VERSION_MAJOR}_viewer PROPERTIES
    COMPILE_FLAGS "-DUSE_POPPLERQT ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
  )

  TARGET_LINK_LIBRARIES(poppler-qt${QT_VERSION_MAJOR}_viewer qtpdf)

  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    ADD_EXECUTABLE(poppler-qt${QT_VERSION_MAJOR}_test
      ${QTPDFTEST_SRCS}
    )
    SET_TARGET_PROPERTIES(poppler-qt${QT_VERSION_MAJOR}_test PROPERTIES
      COMPILE_FLAGS "-DUSE_POPPLERQT ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests
    )
    TARGET_LINK_LIBRARIES(poppler-qt${QT_VERSION_MAJOR}_test qtpdf ${QT_TEST_LIBRARIES})
  ENDIF()
ENDIF()

IF( WITH_MUPDF )
  ADD_EXECUTABLE(mupdf_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_RESOURCES}
  )

  SET_TARGET_PROPERTIES(mupdf_viewer PROPERTIES
    COMPILE_FLAGS "-DUSE_MUPDF ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
  )

  TARGET_LINK_LIBRARIES(mupdf_viewer qtpdf)

  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    ADD_EXECUTABLE(mupdf_test
      ${QTPDFTEST_SRCS}
    )
    SET_TARGET_PROPERTIES(mupdf_test PROPERTIES
      COMPILE_FLAGS "-DUSE_MUPDF ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests
    )
    TARGET_LINK_LIBRARIES(mupdf_test qtpdf ${QT_TEST_LIBRARIES})
  ENDIF()
ENDIF()

ENDIF() # QTPDF_VIEWER

# Packaging
# =========


# Summary
# =======

# This section displays a nice configuration summary for the user.

# These macros borrowed from the Poppler CMake scripts. They add some nice
# formatting to configuration info.
MACRO(CONFIG_INFO what value)
  STRING(LENGTH ${what} length_what)
  MATH(EXPR left_char "35 - ${length_what}")
  SET(blanks)
  FOREACH(_i RANGE 1 ${left_char})
    SET(blanks "${blanks} ")
  ENDFOREACH()

  MESSAGE("  ${what}:${blanks} ${value}")
ENDMACRO()

MACRO(CONFIG_YESNO what enabled)
  IF(${enabled})
    SET(enabled_string "yes")
  ELSE(${enabled})
    SET(enabled_string "no")
  ENDIF()

  CONFIG_INFO("${what}" "${enabled_string}")
ENDMACRO()

# Print out configuration summary.
MESSAGE("${PROJECT_NAME} has been configured:\n")

CONFIG_INFO("Version" ${PROJECT_VERSION})
CONFIG_INFO("Qt version" "${QT_VERSION_MAJOR}.${QT_VERSION_MINOR}.${QT_VERSION_PATCH}")
CONFIG_INFO("Compiler optimization" ${CMAKE_BUILD_TYPE})
message("")

CONFIG_YESNO("Poppler-Qt backend" WITH_POPPLERQT)
CONFIG_YESNO("MuPDF backend" WITH_MUPDF)
CONFIG_YESNO("Shared library" BUILD_SHARED_LIBS)
CONFIG_YESNO("Viewer application" QTPDF_VIEWER)

message("")
message("  ${PROJECT_NAME} will be installed to:")
message("      ${CMAKE_INSTALL_PREFIX}")
message("")


