# Setup and User Options
# ======================

# Set project name.
PROJECT(QtPDF)

# We require CMake v2.8 or greater because we don't do any testing with earlier
# versions.
CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
SET(CMAKE_COLOR_MAKEFILE ON)

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

# ...with poppler-qt4...
IF ( NOT DEFINED WITH_POPPLERQT4 )
  SET(WITH_POPPLERQT4 ON)
ENDIF ()

# ...but without MuPDF
IF ( NOT DEFINED WITH_MUPDF )
  SET(WITH_MUPDF OFF)
ENDIF ()

OPTION(WITH_POPPLER "Build Poppler Qt4 backend" ${WITH_POPPLERQT4})
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

# Declare Qt libraries required by this project.
SET (QT_COMPONENTS
  QtCore
  QtGui
  QtXML
)

# Locate Qt. `INCLUDE(UseQt4)` brings in a set of macros that helps us deal
# with Qt-specific tasks such as compiling resources or running `moc`.
#
# Qt 4.6.0 or newer is required for many pieces of functionality used in the
# code.
FIND_PACKAGE(Qt4 4.6.0 COMPONENTS ${QT_COMPONENTS} REQUIRED)
INCLUDE(UseQt4)

FIND_PACKAGE(ZLIB REQUIRED)

# Aggregate library names and include directories into variables for easy
# access.
SET(QTPDF_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)

SET(QTPDF_LIBS
  ${QT_LIBRARIES}
  ${ZLIB_LIBRARIES}
)


# Backend Components
# ------------------
IF( WITH_POPPLERQT4 )

  FIND_PACKAGE(Poppler REQUIRED)
  LIST(APPEND QTPDF_INCLUDE_DIRS ${POPPLER_QT4_INCLUDE_DIR})
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

SET(QTPDF_MOC_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentTools.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFTransitions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFActions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFAnnotations.h
)

SET(QTPDF_FLAGS "")

IF( WITH_POPPLERQT4 )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQt4Backend.cpp)
  LIST(APPEND QTPDF_MOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQt4Backend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_POPPLERQT4")
ENDIF()

IF( WITH_MUPDF )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.cpp)
  LIST(APPEND QTPDF_MOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_MUPDF")
ENDIF()

QT4_WRAP_CPP(QTPDF_MOC ${QTPDF_MOC_HDRS})

# Create translations
INCLUDE( TranslationMacros )
FILE(GLOB TRANSLATIONS_FILES trans/*.ts)
CREATE_QT4_PRO_FILE("${CMAKE_CURRENT_SOURCE_DIR}/${CMAKE_PROJECT_NAME}.pro" QTPDF_QM ${QTPDF_SRCS} ${QTPDF_MOC_HDRS} ${TRANSLATIONS_FILES})

# Icons
SET(QTPDF_RCS
  ${CMAKE_CURRENT_SOURCE_DIR}/QtPDF_icons.qrc
)
QT4_ADD_RESOURCES(QTPDF_RESOURCES ${QTPDF_RCS})


ADD_LIBRARY(qtpdf
  ${QTPDF_SRCS}
  ${QTPDF_MOC}
  ${QTPDF_RESOURCES}
  ${QTPDF_QM}
)

TARGET_LINK_LIBRARIES(qtpdf ${QTPDF_LIBS})

SET_TARGET_PROPERTIES(qtpdf PROPERTIES
  COMPILE_FLAGS ${QTPDF_FLAGS}
)

# Inform parent scope (if any) what to do to use QtPDF
SET(QTPDF_INCLUDE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src PARENT_SCOPE)
#  ${CMAKE_CURRENT_SOURCE_DIR}/src/backend


# Viewers
# -------

# Both viewers use a common set of source code files. Preprocessor definitions
# toggle the backend-sensitive bits.
SET(PDFVIEWER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PDFViewer.cpp
)

SET(PDFVIEWER_MOC_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/PDFViewer.h
)

SET(PDFVIEWER_RCS
  ${CMAKE_CURRENT_SOURCE_DIR}/icons.qrc
)


SET(QTPDFTEST_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/TestQtPDF.cpp
)

SET(QTPDFTEST_MOC_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/TestQtPDF.h
)


QT4_ADD_RESOURCES(PDFVIEWER_RESOURCES ${PDFVIEWER_RCS})
QT4_WRAP_CPP(PDFVIEWER_MOC ${PDFVIEWER_MOC_HDRS})

QT4_WRAP_CPP(QTPDFTEST_MOC ${QTPDFTEST_MOC_HDRS})


IF( WITH_POPPLERQT4 )

  ADD_EXECUTABLE(poppler-qt4_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_MOC} ${PDFVIEWER_RESOURCES}
  )

  # NOTE:
  # Setting properties on a target sets those properties on all source files
  # that are built to create that target. Other targets can re-use the sources
  # and build with different flags. Pretty handy.
  SET_TARGET_PROPERTIES(poppler-qt4_viewer PROPERTIES
    COMPILE_FLAGS -DUSE_POPPLERQT4
  )

  TARGET_LINK_LIBRARIES(poppler-qt4_viewer qtpdf)


  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    ADD_EXECUTABLE(poppler-qt4_test
      ${QTPDFTEST_SRCS}
      ${QTPDFTEST_MOC}
    )
    SET_TARGET_PROPERTIES(poppler-qt4_test PROPERTIES
      COMPILE_FLAGS -DUSE_POPPLERQT4
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests
    )
    TARGET_LINK_LIBRARIES(poppler-qt4_test qtpdf QtTest)
  ENDIF()

ENDIF()

IF( WITH_MUPDF )

  ADD_EXECUTABLE(mupdf_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_MOC} ${PDFVIEWER_RESOURCES}
  )

  SET_TARGET_PROPERTIES(mupdf_viewer PROPERTIES
    COMPILE_FLAGS -DUSE_MUPDF
  )

  TARGET_LINK_LIBRARIES(mupdf_viewer qtpdf)

  IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
    ADD_EXECUTABLE(mupdf_test
      ${QTPDFTEST_SRCS}
      ${QTPDFTEST_MOC}
    )
    SET_TARGET_PROPERTIES(mupdf_test PROPERTIES
      COMPILE_FLAGS -DUSE_MUPDF
      RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests
    )
    TARGET_LINK_LIBRARIES(mupdf_test qtpdf QtTest)
  ENDIF()

ENDIF()


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
CONFIG_INFO("Compiler optimization" ${CMAKE_BUILD_TYPE})
message("")

CONFIG_YESNO("Poppler-Qt4 backend" WITH_POPPLERQT4)
CONFIG_YESNO("MuPDF backend" WITH_MUPDF)
CONFIG_YESNO("Shared library" BUILD_SHARED_LIBS)

message("")
message("  ${PROJECT_NAME} will be installed to:")
message("      ${CMAKE_INSTALL_PREFIX}")
message("")


