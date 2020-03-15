# Setup and User Options
# ======================

# Set project name.
PROJECT(QtPDF)

# CMake 3.1 significantly improves support for imported targets, Qt5, c++11, etc.
CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
CMAKE_POLICY(VERSION 3.1)

# Silence warning about linking to qtmain.lib statically on Windows
IF(POLICY CMP0020)
  cmake_policy(SET CMP0020 NEW)
ENDIF()

# Silence warning about using @rpath on OS X.
if(POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()

# Silence warning about ninja custom command byproducts
if(POLICY CMP0058)
  cmake_policy(SET CMP0058 NEW)
endif()

# Silence warning about option() treating variables differently on the first run
if(POLICY CMP0077)
  cmake_policy(SET CMP0077 NEW)
endif()

SET(CMAKE_COLOR_MAKEFILE ON)
SET(CMAKE_AUTOMOC TRUE)
SET(CMAKE_AUTORCC TRUE)

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
OPTION(BUILD_SHARED_LIBS "Build shared library" ON)

# ...with the viewer program...
OPTION(QTPDF_VIEWER "Build PDF viewer application" ON)

# ...with tests...
OPTION(WITH_TESTS "Build tests" ON)

# ...with poppler-qt...
OPTION(WITH_POPPLERQT "Build Poppler Qt backend" ON)
# ...but without MuPDF
# MuPDF backend is a bit immature, so we don't bother with it by default right
# now.
OPTION(WITH_MUPDF "Build MuPDF backend" OFF)



# Dependency Configuration
# ========================

# Make the contents of `CMake/Modules` available. Among other things, this
# directory contains scripts that locate project components such as Poppler.
LIST(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMake/Modules)

set (CMAKE_CXX_STANDARD 11)

# Core Components
# ---------------

find_package(Qt5 REQUIRED COMPONENTS Core Widgets Concurrent Xml LinguistTools)

if (WITH_TESTS)
  find_package(Qt5 OPTIONAL_COMPONENTS Test QUIET)
  if (NOT Qt5Test_FOUND)
    set(WITH_TESTS OFF)
  endif()
endif()

if (WIN32 AND NOT BUILD_SHARED_LIBS)
  find_package(Qt5WindowsPlatformSupport)
endif ()

# Note: Qt5 only sets Qt5Widgets_VERSION, etc., but not QT_VERSION_MAJOR,
# etc. which is used here.
string(REGEX REPLACE "^([0-9]+).*$" "\\1" QT_VERSION_MAJOR "${Qt5Widgets_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_MINOR  "${Qt5Widgets_VERSION}")
string(REGEX REPLACE "^[0-9]+\\.[0-9]+\\.([0-9]+).*$" "\\1" QT_VERSION_PATCH "${Qt5Widgets_VERSION}")

# Expose the major version number of Qt to the preprocessor. This is necessary
# to include the correct Qt headers (as QTVERSION is not defined before any Qt
# headers are included)
ADD_DEFINITIONS(-DQT_VERSION_MAJOR=${QT_VERSION_MAJOR})


FIND_PACKAGE(ZLIB REQUIRED)

SET(QTPDF_LIBS
  ${QT_PLATFORM_LIBRARIES}
  Qt5::Core Qt5::Widgets Qt5::Concurrent Qt5::Core Qt5::Xml
  ZLIB::ZLIB
)


# Backend Components
# ------------------
IF( WITH_POPPLERQT )
  FIND_PACKAGE(Poppler REQUIRED COMPONENTS qt5)

  # As in FindPoppler.cmake, it is important to list poppler after Qt to avoid
  # confusion with dependencies when using statically linked libraries
  SET(QTPDF_LIBS ${QTPDF_LIBS} Poppler::poppler-qt5)

  # Check if Poppler provides certain features (as poppler does not reliably
  # expose its version number, there is no easy way to check for these at
  # compile-time)
  include(CheckCXXSourceCompiles)
  set(CMAKE_REQUIRED_LIBRARIES Poppler::poppler-qt5)

  # Page::SearchFlags were added in 0.31
  CHECK_CXX_SOURCE_COMPILES("#include <poppler-qt${QT_VERSION_MAJOR}.h>\nint main() { double x; Poppler::Document::load(QString())->page(0)->search(QString(), x, x, x, x, Poppler::Page::NextResult, Poppler::Page::IgnoreCase); return 0; }" POPPLER_HAS_SEARCH_FLAGS)
  if (POPPLER_HAS_SEARCH_FLAGS)
    add_definitions(-DPOPPLER_HAS_SEARCH_FLAGS)
  endif (POPPLER_HAS_SEARCH_FLAGS)

  # PageTransition::durationReal() was added in 0.37
  CHECK_CXX_SOURCE_COMPILES("#include <poppler-qt${QT_VERSION_MAJOR}.h>\nint main() { Poppler::Document::load(QString())->page(0)->transition()->durationReal(); return 0; }" POPPLER_HAS_DURATION_REAL)
  if (POPPLER_HAS_DURATION_REAL)
    add_definitions(-DPOPPLER_HAS_DURATION_REAL)
  endif (POPPLER_HAS_DURATION_REAL)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src
  ${CMAKE_CURRENT_SOURCE_DIR}/src/backend
  SYSTEM ${QTPDF_INCLUDE_DIRS}
)

IF( ${CMAKE_BUILD_TYPE} STREQUAL "Debug" )
  ADD_DEFINITIONS(-DDEBUG -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)
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
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PaperSizes.cpp
)

SET(QTPDF_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentView.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentWidget.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFDocumentTools.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFBackend.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFTransitions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFActions.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PDFAnnotations.h
  ${CMAKE_CURRENT_SOURCE_DIR}/src/PaperSizes.h
)

# FIXME: Is -fPIC required/appropriate for all situations/platforms?
IF( NOT WIN32 )
  SET(QTPDF_FLAGS "-fPIC")
ENDIF()

IF( WITH_POPPLERQT )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQtBackend.cpp)
  LIST(APPEND QTPDF_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/PopplerQtBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_POPPLERQT")

  get_target_property(POPPLER_HAS_XPDF Poppler::poppler HAS_PRIVATE_HEADERS)
  if (POPPLER_HAS_XPDF)
    set(QTPDF_FLAGS "${QTPDF_FLAGS} -DHAVE_POPPLER_XPDF_HEADERS")

    set(CMAKE_REQUIRED_LIBRARIES Poppler::poppler-qt5)

    # GlobalParamsIniter was introduced in 0.85
    CHECK_CXX_SOURCE_COMPILES("#include <GlobalParams.h>\nint main() { GlobalParamsIniter::setCustomDataDir(\"\"); return 0; }" POPPLER_HAS_GLOBALPARAMSINITER)
    if (POPPLER_HAS_GLOBALPARAMSINITER)
      add_definitions(-DPOPPLER_HAS_GLOBALPARAMSINITER)
    endif (POPPLER_HAS_GLOBALPARAMSINITER)

    # GlobalParams was turned into a unique_ptr in 0.83
    CHECK_CXX_SOURCE_COMPILES("#include <GlobalParams.h>\n#include <memory>\nint main() { globalParams = std::move(std::unique_ptr<GlobalParams>(new GlobalParams())); return 0; }" POPPLER_GLOBALPARAMS_IS_UNIQUE)
    if (POPPLER_GLOBALPARAMS_IS_UNIQUE)
      add_definitions(-DPOPPLER_GLOBALPARAMS_IS_UNIQUE)
    endif (POPPLER_GLOBALPARAMS_IS_UNIQUE)
  endif (POPPLER_HAS_XPDF)
ENDIF()

IF( WITH_MUPDF )
  LIST(APPEND QTPDF_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.cpp)
  LIST(APPEND QTPDF_HDRS ${CMAKE_CURRENT_SOURCE_DIR}/src/backends/MuPDFBackend.h)
  SET(QTPDF_FLAGS "${QTPDF_FLAGS} -DUSE_MUPDF")
ENDIF()


# Icons
SET(QTPDF_RCS
  ${CMAKE_CURRENT_SOURCE_DIR}/QtPDF_icons.qrc
)


ADD_LIBRARY(qtpdf ${QTPDF_LIB_TYPE}
  ${QTPDF_SRCS}
  ${QTPDF_HDRS}
  ${QTPDF_RCS}
  ${QTPDF_QM}
)

# Create translations
include(TranslationMacros)
file(GLOB TRANSLATIONS_SOURCES trans/*.ts)
target_add_qt_translations(qtpdf PRO_FILE "${CMAKE_CURRENT_SOURCE_DIR}/trans/QtPDF_trans.pro" TS_FILES ${TRANSLATIONS_SOURCES})


TARGET_LINK_LIBRARIES(qtpdf ${QTPDF_LIBS})

SET_TARGET_PROPERTIES(qtpdf PROPERTIES
  COMPILE_FLAGS "${QTPDF_FLAGS}"
  INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if (MSVC)
  target_compile_options(qtpdf PRIVATE /W4)
else ()
  target_compile_options(qtpdf PRIVATE -Wall -Wpedantic -Wextra -Wconversion -Wold-style-cast -Woverloaded-virtual -Wzero-as-null-pointer-constant)
endif ()

if (NOT TARGET QtPDF::qtpdf)
  add_library(QtPDF::qtpdf ALIAS qtpdf)
endif ()


# Viewers
# -------

IF ( QTPDF_VIEWER )

# Both viewers use a common set of source code files. Preprocessor definitions
# toggle the backend-sensitive bits.
SET(PDFVIEWER_SRCS
  ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/PDFViewer.cpp
)

SET(PDFVIEWER_HDRS
  ${CMAKE_CURRENT_SOURCE_DIR}/PDFViewer.h
)

IF( WITH_POPPLERQT )
  ADD_EXECUTABLE(poppler-qt${QT_VERSION_MAJOR}_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_HDRS}
    icons.qrc
  )

  # NOTE:
  # Setting properties on a target sets those properties on all source files
  # that are built to create that target. Other targets can re-use the sources
  # and build with different flags. Pretty handy.
  SET_TARGET_PROPERTIES(poppler-qt${QT_VERSION_MAJOR}_viewer PROPERTIES
    COMPILE_FLAGS "-DUSE_POPPLERQT ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
  )

  TARGET_LINK_LIBRARIES(poppler-qt${QT_VERSION_MAJOR}_viewer qtpdf)

ENDIF()

IF( WITH_MUPDF )
  ADD_EXECUTABLE(mupdf_viewer
    ${PDFVIEWER_SRCS}
    ${PDFVIEWER_HDRS}
    icons.qrc
  )

  SET_TARGET_PROPERTIES(mupdf_viewer PROPERTIES
    COMPILE_FLAGS "-DUSE_MUPDF ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
  )

  TARGET_LINK_LIBRARIES(mupdf_viewer qtpdf)
ENDIF()

ENDIF() # QTPDF_VIEWER

# Tests
# -----

IF ( WITH_TESTS )
  ENABLE_TESTING(TRUE)
  SET(QTPDFTEST_SRCS
    ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/TestQtPDF.cpp
  )
  SET(QTPDFTEST_HDRS
    ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests/TestQtPDF.h
  )
  IF( WITH_POPPLERQT )
    ADD_EXECUTABLE(test_poppler-qt${QT_VERSION_MAJOR}
      ${QTPDFTEST_SRCS}
      ${QTPDFTEST_HDRS}
    )
    SET_TARGET_PROPERTIES(test_poppler-qt${QT_VERSION_MAJOR} PROPERTIES
      COMPILE_FLAGS "-DUSE_POPPLERQT ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
    )
    target_link_libraries(test_poppler-qt${QT_VERSION_MAJOR} Qt5::Test qtpdf)
    ADD_TEST(NAME test_poppler-qt${QT_VERSION_MAJOR} COMMAND test_poppler-qt${QT_VERSION_MAJOR} WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests)
  ENDIF()
  IF( WITH_MUPDF )
    ADD_EXECUTABLE(test_mupdf
      ${QTPDFTEST_SRCS}
      ${QTPDFTEST_HDRS}
    )
    SET_TARGET_PROPERTIES(test_mupdf PROPERTIES
      COMPILE_FLAGS "-DUSE_MUPDF ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}"
    )
    target_link_libraries(test_mupdf Qt5::Test qtpdf)
    ADD_TEST(NAME test_mupdf COMMAND test_mupdf WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/unit-tests)
  ENDIF()
ENDIF( WITH_TESTS )


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


