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

OPTION(WITH_POPPLER "Build Poppler backend" ON)
# MuPDF backend is a bit immature, so we don't bother with it by default right
# now.
OPTION(WITH_MUPDF "Build MuPDF backend" OFF)



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
IF( WITH_POPPLER )

  FIND_PACKAGE(Poppler REQUIRED)
  LIST(APPEND QTPDF_INCLUDE_DIRS ${POPPLER_QT4_INCLUDE_DIR})
  LIST(APPEND QTPDF_LIBS ${POPPLER_LIBRARIES})

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
  ADD_DEFINITIONS(-DDEBUG)
ENDIF()


# Library
# -------
SET(QTPDF_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.cpp
)

SET(QTPDF_MOC_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.h
)

SET(QTPDF_FLAGS "")

IF( WITH_POPPLER )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerBackend.cpp)
  LIST(APPEND QTPDF_MOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_POPPLER")
ENDIF()

IF( WITH_MUPDF )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.cpp)
  LIST(APPEND QTPDF_MOC_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_MUPDF")
ENDIF()

QT4_WRAP_CPP(QTPDF_MOC ${QTPDF_MOC_HDRS})

ADD_LIBRARY(qtpdf SHARED
  ${QTPDF_SRCS}
  ${QTPDF_MOC}
)

TARGET_LINK_LIBRARIES(qtpdf ${QTPDF_LIBS})

SET_TARGET_PROPERTIES(qtpdf PROPERTIES
  COMPILE_FLAGS ${QTPDF_FLAGS}
)



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

QT4_ADD_RESOURCES(PDFVIEWER_RESOURCES ${PDFVIEWER_RCS})
QT4_WRAP_CPP(PDFVIEWER_MOC ${PDFVIEWER_MOC_HDRS})


IF( WITH_POPPLER )

  ADD_EXECUTABLE(poppler_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_MOC} ${PDFVIEWER_RESOURCES}
  )

  # NOTE:
  # Setting properties on a target sets those properties on all source files
  # that are built to create that target. Other targets can re-use the sources
  # and build with different flags. Pretty handy.
  SET_TARGET_PROPERTIES(poppler_viewer PROPERTIES
    COMPILE_FLAGS -DUSE_POPPLER
  )

  TARGET_LINK_LIBRARIES(poppler_viewer qtpdf)

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
MESSAGE("${CMAKE_PROJECT_NAME} has been configured:\n")

CONFIG_INFO("Version" ${PROJECT_VERSION})
CONFIG_INFO("Compiler optimization" ${CMAKE_BUILD_TYPE})
message("")

CONFIG_YESNO("Poppler backend" WITH_POPPLER)
CONFIG_YESNO("MuPDF backend" WITH_MUPDF)

message("")
message("  Project will be installed to:")
message("      ${CMAKE_INSTALL_PREFIX}")
message("")


