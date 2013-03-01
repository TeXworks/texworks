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



# Dependency Configuration
# ========================

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as Poppler.
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules)

# Declare Qt libraries required by this project.
SET (QT_COMPONENTS
  QtCore
  QtGui
)


# Locate Qt. `INCLUDE(UseQt4)` brings in a set of macros that helps us deal
# with Qt-specific tasks such as compiling resources or running `moc`.
FIND_PACKAGE(Qt4 COMPONENTS ${QT_COMPONENTS} REQUIRED)
INCLUDE(UseQt4)

FIND_PACKAGE(ZLIB REQUIRED)
FIND_PACKAGE(Poppler REQUIRED)

# The only thing Poppler should need is the location of the include directories
# in order to access header files. The library loader should be able to find
# libfontconfig on standard search paths.
IF ( POPPLER_NEEDS_FONTCONFIG )
  FIND_PACKAGE(Fontconfig REQUIRED)
ENDIF ()

# Aggregate library names and include directories into variables for easy
# access.
SET(QTPDF_INCLUDE_DIRS
  ${QT_INCLUDE_DIR}
  ${HUNSPELL_INCLUDE_DIR}
  ${POPPLER_QT4_INCLUDE_DIR}
  ${ZLIB_INCLUDE_DIR}
)
IF ( POPPLER_NEEDS_FONTCONFIG )
  LIST(APPEND QTPDF_INCLUDE_DIRS ${FONTCONFIG_INCLUDE_DIR})
ENDIF ()

SET(QTPDF_LIBS
  ${QT_LIBRARIES}
  ${POPPLER_LIBRARIES}
  ${ZLIB_LIBRARIES}
)


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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerBackend.cpp
  #  ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.cpp
)

SET(QTPDF_MOC_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerBackend.h
  #  ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.h
)

QT4_WRAP_CPP(QTPDF_MOC ${QTPDF_MOC_HDRS})

ADD_LIBRARY(qtpdf SHARED
  ${QTPDF_SRCS}
  ${QTPDF_MOC}
)

SET_TARGET_PROPERTIES(qtpdf PROPERTIES
  COMPILE_FLAGS -DUSE_POPPLER
)

TARGET_LINK_LIBRARIES(qtpdf ${QTPDF_LIBS})


# Viewer
# ------
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

ADD_EXECUTABLE(pdf_viewer
  ${PDFVIEWER_SRCS}
  ${PDFVIEWER_MOC} ${PDFVIEWER_RESOURCES}
)

SET_TARGET_PROPERTIES(pdf_viewer PROPERTIES
  COMPILE_FLAGS -DUSE_POPPLER
)

TARGET_LINK_LIBRARIES(pdf_viewer qtpdf)



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

CONFIG_YESNO("Poppler backend" POPPLER_FOUND)

message("")
message("  Project will be installed to:")
message("      ${CMAKE_INSTALL_PREFIX}")
message("")


