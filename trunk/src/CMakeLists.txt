# Build TeXworks
# ==============

# Collect Source Files
# --------------------

# **NOTE:** _A `GLOB` operation is quick, but may not be appropriate. It may be
# better to maintain an explicit list of source files. Particularly, adding a
# new file to an explicit list will cause CMake to automatically reconfigure
# the next time `make` is invoked. When `GLOB` is used, the developer will have
# to remember to re-run `cmake` if a source file is added._
FILE(GLOB TEXWORKS_SRCS       "*.cpp" "*.c")
FILE(GLOB TEXWORKS_HDRS       "*.h")
FILE(GLOB TEXWORKS_UIS        "*.ui")
FILE(GLOB TEXWORKS_RCS        "${TeXworks_SOURCE_DIR}/res/*.qrc")
FILE(GLOB TEXWORKS_TRANS_TS   "${TeXworks_SOURCE_DIR}/trans/*.ts")
FILE(GLOB TEXWORKS_TRANS_QM   "${TeXworks_SOURCE_DIR}/trans/*.qm" "${TeXworks_SOURCE_DIR}/trans/qt/*.qm")

IF ( WIN32 )
	FILE(GLOB TEXWORKS_WIN_RCS  "${TeXworks_SOURCE_DIR}/res/*.rc")
  LIST(APPEND TEXWORKS_SRCS ${TEXWORKS_WIN_RCS})
ENDIF ( WIN32 )

# These source files along with the `.moc` files generated from some headers
# form the TeXworks scripting API. We export them to variables in the
# `PARENT_SCOPE` (top level CMake file) so that they will be available to the
# build processes of the scripting language plugins.
SET(TeXworks_SCRIPT_API
  ${CMAKE_CURRENT_SOURCE_DIR}/TWScript.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/TWScriptAPI.cpp
  PARENT_SCOPE
)

SET(TeXworks_SCRIPT_API_H
  ${CMAKE_CURRENT_SOURCE_DIR}/TWScript.h
  ${CMAKE_CURRENT_SOURCE_DIR}/TWScriptAPI.h
  ${CMAKE_CURRENT_SOURCE_DIR}/TWSystemCmd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/ConfigurableApp.h
  PARENT_SCOPE
)


# App Setup
# ---------

# On OS X, set GUI type to an `.app` bundle, gather icon files and add core system
# frameworks to the library link list.
IF ( APPLE )
  SET(GUI_TYPE MACOSX_BUNDLE)

  LIST(APPEND TeXworks_LIBS  "-framework CoreServices")

  FILE(GLOB TeXworks_APP_ICONS ${TeXworks_SOURCE_DIR}/*.icns)
  SET_SOURCE_FILES_PROPERTIES( ${TeXworks_APP_ICONS}
    PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
  )
ENDIF ()

# **WARNING:** _Windows is untested_
IF ( WIN32 )
  SET(GUI_TYPE WIN32)
ENDIF ()


# Make sure the compiler can find include files.
INCLUDE_DIRECTORIES(${TeXworks_INCLUDE_DIRS})

# Make sure the compiler can find library files.
LINK_DIRECTORIES(${TeXworks_LIB_DIRS})

ADD_DEFINITIONS(-DTW_BUILD_ID=${TW_BUILD_ID})
IF (UNIX AND NOT APPLE)
  IF (IS_ABSOLUTE ${TeXworks_HELP_DIR})
    ADD_DEFINITIONS(-DTW_HELPPATH="${TeXworks_HELP_DIR}")
  ELSE ()
    ADD_DEFINITIONS(-DTW_HELPPATH="${CMAKE_INSTALL_PREFIX}/${TeXworks_HELP_DIR}")
  ENDIF ()
  IF (IS_ABSOLUTE ${TeXworks_PLUGIN_DIR})
    ADD_DEFINITIONS(-DTW_PLUGINPATH="${TeXworks_PLUGIN_DIR}")
  ELSE ()
    ADD_DEFINITIONS(-DTW_PLUGINPATH="${CMAKE_INSTALL_PREFIX}/${TeXworks_PLUGIN_DIR}")
  ENDIF ()
  IF (IS_ABSOLUTE ${TeXworks_DIC_DIR})
    ADD_DEFINITIONS(-DTW_DICPATH="${TeXworks_DIC_DIR}")
  ELSE ()
    ADD_DEFINITIONS(-DTW_DICPATH="${CMAKE_INSTALL_PREFIX}/${TeXworks_DIC_DIR}")
  ENDIF ()
ENDIF ()

# Poppler supports XPDF API calls?
IF (POPPLER_HAS_XPDF)
  INCLUDE_DIRECTORIES(${POPPLER_XPDF_INCLUDE_DIR})
  ADD_DEFINITIONS(-DHAVE_POPPLER_XPDF_HEADERS)
ENDIF ()


# Qt-Specific Source Code Processing
# ----------------------------------

IF (${QT_VERSION_MAJOR} EQUAL 5)
  # Run uic on user interface files.
  QT5_WRAP_UI(TEXWORKS_GEN_UI ${TEXWORKS_UIS})
ELSE()
  # Run uic on user interface files.
  QT4_WRAP_UI(TEXWORKS_GEN_UI ${TEXWORKS_UIS})
ENDIF()
# Run rcc on reasource files.
QT_ADD_RESOURCES(TEXWORKS_GEN_RCS ${TEXWORKS_RCS})
# Add translations
QT_ADD_QM_TRANSLATIONS(TEXWORKS_GEN_TRANS ${TEXWORKS_TRANS_QM})

IF ( APPLE OR NOT UNIX )
  SET(EXE_NAME TeXworks)
ELSE ( APPLE OR NOT UNIX )
  SET(EXE_NAME texworks)
ENDIF ( APPLE OR NOT UNIX )

# Compilation
# -----------

IF (NOT ${BUILD_SHARED_LIBS})
  IF (WITH_LUA)
    LIST(APPEND TeXworks_LIBS TWLuaPlugin)
  ENDIF()
  IF (WITH_PYTHON)
    LIST(APPEND TeXworks_LIBS TWPythonPlugin)
  ENDIF()
ENDIF()

# Build the executible.
ADD_EXECUTABLE( ${EXE_NAME} ${GUI_TYPE}
  ${TEXWORKS_SRCS}
  ${TEXWORKS_GEN_UI} ${TEXWORKS_GEN_RCS} ${TEXWORKS_GEN_TRANS}
  ${TeXworks_APP_ICONS}
)

# Specify libraries to be linked to TeXworks.
TARGET_LINK_LIBRARIES( ${EXE_NAME} ${TeXworks_LIBS} )

# For OS X, generate `Info.plist` and add it to the `.app` bundle.
IF (APPLE)
  SET(VER ${TeXworks_VERSION})
  SET(REV ${TeXworks_SVN_REV})
  CONFIGURE_FILE(${TeXworks_SOURCE_DIR}/TeXworks.plist.in
    ${TeXworks_SOURCE_DIR}/TeXworks.plist
    @ONLY
  )

  SET_TARGET_PROPERTIES( TeXworks PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
      ${TeXworks_SOURCE_DIR}/TeXworks.plist
  )
ENDIF ()

IF (${QT_VERSION_MAJOR} EQUAL 5 AND UNIX)
    SET_TARGET_PROPERTIES(${EXE_NAME} PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF ()

# Installation
# ------------
INSTALL( TARGETS ${EXE_NAME}
  RUNTIME DESTINATION bin COMPONENT
  BUNDLE DESTINATION . COMPONENT
)

IF ( UNIX AND NOT APPLE )
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR})
  INSTALL(FILES
    ${TeXworks_SOURCE_DIR}/COPYING
    ${TeXworks_SOURCE_DIR}/README
    ${TeXworks_SOURCE_DIR}/NEWS
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_DOCS_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/man/texworks.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_MAN_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/texworks.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_DESKTOP_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/texworks.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_APPDATA_DIR})
ENDIF ( UNIX AND NOT APPLE )

# On OS X, ensure there is always a `qt.conf` file as all hell will break loose
# if one isn't there.  This may need to be customized for use on non-Apple
# systems.
#
# Protip: If there are wierd errors like "Qt libraries loaded twice" or
# "looking for a tablet device", then something may have gone pear shaped with
# `qt.conf`.
IF ( APPLE )
  SET(QTCONF_DIR ${PROJECT_NAME}.app/Contents/Resources)
  SET(QTCONF_CONTENTS "[Paths]\nPlugins = PlugIns")

  INSTALL( CODE
    "
    FILE(WRITE \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/${QTCONF_DIR}/qt.conf\"
      \"${QTCONF_CONTENTS}\"
    )
    "
  )
ENDIF()
