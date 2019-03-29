# Build TeXworks
# ==============

# Collect Source Files
# --------------------

# **NOTE:** _A `GLOB` operation is quick, but may not be appropriate. It may be
# better to maintain an explicit list of source files. Particularly, adding a
# new file to an explicit list will cause CMake to automatically reconfigure
# the next time `make` is invoked. When `GLOB` is used, the developer will have
# to remember to re-run `cmake` if a source file is added._
set(TEXWORKS_SRCS BibTeXFile.cpp
                  CitationSelectDialog.cpp
                  ClickableLabel.cpp
                  ClosableTabWidget.cpp
                  CommandlineParser.cpp
                  CompletingEdit.cpp
                  ConfigurableApp.cpp
                  ConfirmDelete.cpp
                  Engine.cpp
                  FindDialog.cpp
                  HardWrapDialog.cpp
                  main.cpp
                  PDFDocument.cpp
                  PrefsDialog.cpp
                  ResourcesDialog.cpp
                  ScreenCalibrationWidget.cpp
                  ScriptManager.cpp
                  TemplateDialog.cpp
                  TeXDocks.cpp
                  TeXDocument.cpp
                  TeXHighlighter.cpp
                  TWApp.cpp
                  TWScriptable.cpp
                  TWScriptAPI.cpp
                  TWScript.cpp
                  TWSynchronizer.cpp
                  TWSystemCmd.cpp
                  TWTextCodecs.cpp
                  TWUtils.cpp)

set(TEXWORKS_HDRS BibTeXFile.h
                  CitationSelectDialog.h
                  ClickableLabel.h
                  ClosableTabWidget.h
                  CommandlineParser.h
                  CompletingEdit.h
                  ConfigurableApp.h
                  ConfirmDelete.h
                  DefaultPrefs.h
                  Engine.h
                  FindDialog.h
                  GitRev.h
                  HardWrapDialog.h
                  PDFDocument.h
                  PrefsDialog.h
                  ResourcesDialog.h
                  ScreenCalibrationWidget.h
                  ScriptManager.h
                  TemplateDialog.h
                  TeXDocks.h
                  TeXDocument.h
                  TeXHighlighter.h
                  TWApp.h
                  TWScriptable.h
                  TWScriptAPI.h
                  TWScript.h
                  TWSynchronizer.h
                  TWSystemCmd.h
                  TWTextCodecs.h
                  TWUtils.h
                  TWVersion.h)

if (WIN32)
  list(APPEND TEXWORKS_HDRS DefaultBinaryPathsWin.h)
else ()
  list(APPEND TEXWORKS_HDRS DefaultBinaryPaths.h)
endif ()

set(TEXWORKS_UIS CitationSelectDialog.ui
                 ConfirmDelete.ui
                 Find.ui
                 HardWrapDialog.ui
                 PDFDocument.ui
                 PDFFind.ui
                 PrefsDialog.ui
                 Replace.ui
                 ResourcesDialog.ui
                 ScriptManager.ui
                 SearchResults.ui
                 TemplateDialog.ui
                 TeXDocument.ui
                 ToolConfig.ui)

set(TEXWORKS_RCS ${TeXworks_SOURCE_DIR}/res/resfiles.qrc
                 ${TeXworks_SOURCE_DIR}/res/resources.qrc)

FILE(GLOB TEXWORKS_TRANS_TS   "${TeXworks_SOURCE_DIR}/trans/*.ts")
FILE(GLOB QT_TRANS_QM         "${TeXworks_SOURCE_DIR}/trans/qt/*.qm")

IF ( WIN32 )
  LIST(APPEND TEXWORKS_SRCS ${TeXworks_SOURCE_DIR}/res/TeXworks.rc)
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


# Disallow automatic casts from char* to QString (enforcing the use of tr() or
# explicitly specifying the string encoding)
ADD_DEFINITIONS(-DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY -DQT_STRICT_ITERATORS)

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


# Compilation
# -----------

IF (NOT ${BUILD_SHARED_PLUGINS})
  IF (WITH_LUA AND LUA_FOUND)
    LIST(INSERT TeXworks_LIBS 0 TWLuaPlugin ${LUA_LIBRARIES})
  ENDIF()
  IF (WITH_PYTHON AND PYTHONLIBS_FOUND)
    LIST(INSERT TeXworks_LIBS 0 TWPythonPlugin ${PYTHON_LIBRARIES})
  ENDIF()
ENDIF()

# Build the executable
ADD_EXECUTABLE(TeXworks ${GUI_TYPE}
  ${TEXWORKS_SRCS} ${TEXWORKS_HDRS} ${TEXWORKS_UIS}
  ${TEXWORKS_RCS}
  ${TeXworks_APP_ICONS}
)

if (UNIX AND NOT APPLE)
  set_target_properties(TeXworks PROPERTIES
    OUTPUT_NAME texworks
  )
endif ()

# Add translations
target_add_qt_translations(TeXworks
  PRO_FILE "${CMAKE_SOURCE_DIR}/trans/${PROJECT_NAME}_trans.pro"
  TS_FILES ${TEXWORKS_TRANS_TS}
  QM_FILES ${QT_TRANS_QM}
)

IF (UNIX)
  # Required (at least) on OpenBSD to make scripting plugins work
  SET_TARGET_PROPERTIES(TeXworks PROPERTIES LINK_FLAGS -rdynamic)
ENDIF ()

# Specify libraries to be linked to TeXworks.
target_LINK_LIBRARIES(TeXworks QtPDF::qtpdf ${TeXworks_LIBS} )

# For OS X, generate `Info.plist` and add it to the `.app` bundle.
IF (APPLE)
  SET(VER ${TeXworks_VERSION})
  SET(REV ${TeXworks_GIT_HASH})
  CONFIGURE_FILE(${TeXworks_SOURCE_DIR}/TeXworks.plist.in
    ${TeXworks_SOURCE_DIR}/TeXworks.plist
    @ONLY
  )

  SET_TARGET_PROPERTIES( TeXworks PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
      ${TeXworks_SOURCE_DIR}/TeXworks.plist
  )
ENDIF ()

IF (UNIX)
    SET_TARGET_PROPERTIES(TeXworks PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF ()

# Installation
# ------------
INSTALL(TARGETS TeXworks
  RUNTIME DESTINATION bin COMPONENT
  BUNDLE DESTINATION . COMPONENT
)

IF ( UNIX AND NOT APPLE )
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR})
  INSTALL(FILES
    ${TeXworks_SOURCE_DIR}/COPYING
    ${TeXworks_SOURCE_DIR}/README.md
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
