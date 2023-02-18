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
                  CompletingEdit.cpp
                  Engine.cpp
                  FindDialog.cpp
                  HardWrapDialog.cpp
                  main.cpp
                  PDFDocumentWindow.cpp
                  PrefsDialog.cpp
                  ResourcesDialog.cpp
                  ScriptManagerWidget.cpp
                  Settings.cpp
                  TemplateDialog.cpp
                  TeXDocks.cpp
                  TeXDocumentWindow.cpp
                  TeXHighlighter.cpp
                  TWApp.cpp
                  TWScriptableWindow.cpp
                  TWScriptManager.cpp
                  TWSynchronizer.cpp
                  TWUtils.cpp
                  document/Document.cpp
                  document/SpellChecker.cpp
                  document/TextDocument.cpp
                  document/TeXDocument.cpp
                  scripting/ECMAScriptInterface.cpp
                  scripting/ECMAScript.cpp
                  scripting/ScriptAPI.cpp
                  scripting/Script.cpp
                  ui/ClickableLabel.cpp
                  ui/ClosableTabWidget.cpp
                  ui/ColorButton.cpp
                  ui/LineNumberWidget.cpp
                  ui/ListSelectDialog.cpp
                  ui/RemoveAuxFilesDialog.cpp
                  ui/ScreenCalibrationWidget.cpp
                  utils/CmdKeyFilter.cpp
                  utils/CommandlineParser.cpp
                  utils/FileVersionDatabase.cpp
                  utils/FullscreenManager.cpp
                  utils/ResourcesLibrary.cpp
                  utils/SystemCommand.cpp
                  utils/TextCodecs.cpp
                  utils/TypesetManager.cpp
                  utils/VersionInfo.cpp
                  utils/WindowManager.cpp
                  )

set(TEXWORKS_HDRS BibTeXFile.h
                  CitationSelectDialog.h
                  CompletingEdit.h
                  DefaultBinaryPaths.h
                  DefaultPrefs.h
                  Engine.h
                  FindDialog.h
                  GitRev.h
                  HardWrapDialog.h
                  PDFDocumentWindow.h
                  PrefsDialog.h
                  ResourcesDialog.h
                  ScriptManagerWidget.h
                  Settings.h
                  TemplateDialog.h
                  TeXDocks.h
                  TeXDocumentWindow.h
                  TeXHighlighter.h
                  TWApp.h
                  TWScriptableWindow.h
                  TWScriptManager.h
                  TWSynchronizer.h
                  TWUtils.h
                  TWVersion.h
                  InterProcessCommunicator.h
                  document/Document.h
                  document/SpellChecker.h
                  document/TextDocument.h
                  document/TeXDocument.h
                  scripting/ScriptAPIInterface.h
                  scripting/ScriptLanguageInterface.h
                  scripting/ScriptAPI.h
                  scripting/Script.h
                  ui/ClickableLabel.h
                  ui/ClosableTabWidget.h
                  ui/ColorButton.h
                  ui/LineNumberWidget.h
                  ui/ListSelectDialog.h
                  ui/RemoveAuxFilesDialog.h
                  ui/ScreenCalibrationWidget.h
                  utils/CmdKeyFilter.cpp
                  utils/CommandlineParser.h
                  utils/FileVersionDatabase.h
                  utils/FullscreenManager.h
                  utils/ResourcesLibrary.h
                  utils/SystemCommand.h
                  utils/TextCodecs.h
                  utils/TypesetManager.h
                  utils/VersionInfo.h
                  utils/WindowManager.h
                  )

if (WIN32)
  list(APPEND TEXWORKS_SRCS InterProcessCommunicatorWin.cpp)
elseif (APPLE)
  list(APPEND TEXWORKS_SRCS InterProcessCommunicatorDummy.cpp)
  list(APPEND TEXWORKS_SRCS utils/OSVersionInfo_mac.mm)
else ()
  list(APPEND TEXWORKS_SRCS InterProcessCommunicatorDBus.cpp)
endif ()

if (WITH_QTSCRIPT)
  list(APPEND TEXWORKS_SRCS scripting/JSScriptInterface.cpp scripting/JSScript.cpp)
  list(APPEND TEXWORKS_HDRS scripting/JSScriptInterface.h scripting/JSScript.h)
  add_definitions(-DWITH_QTSCRIPT)
endif (WITH_QTSCRIPT)

set(TEXWORKS_UIS CitationSelectDialog.ui
                 CompletingEdit.ui
                 Find.ui
                 HardWrapDialog.ui
                 PDFDocumentWindow.ui
                 PDFFind.ui
                 PrefsDialog.ui
                 Replace.ui
                 ResourcesDialog.ui
                 ScriptManagerWidget.ui
                 SearchResults.ui
                 TemplateDialog.ui
                 TeXDocumentWindow.ui
                 ToolConfig.ui
                 ui/ListSelectDialog.ui
                 )

set(TEXWORKS_RCS ${TeXworks_SOURCE_DIR}/res/resfiles.qrc
                 ${TeXworks_SOURCE_DIR}/res/resources.qrc)

FILE(GLOB TEXWORKS_TRANS_TS   "${TeXworks_SOURCE_DIR}/trans/*.ts")
FILE(GLOB QT_TRANS_QM         "${TeXworks_SOURCE_DIR}/trans/qt/*.qm")

IF ( WIN32 )
  if (NOT MSVC)
    set(CREATE_MANIFEST TRUE)
  endif (NOT MSVC)
  configure_file(${TeXworks_SOURCE_DIR}/res/TeXworks.rc TeXworks.rc)
  list(APPEND TEXWORKS_SRCS ${CMAKE_CURRENT_BINARY_DIR}/TeXworks.rc)
ENDIF ( WIN32 )

# These source files along with the `.moc` files generated from some headers
# form the TeXworks scripting API. We export them to variables in the
# `PARENT_SCOPE` (top level CMake file) so that they will be available to the
# build processes of the scripting language plugins.
SET(TeXworks_SCRIPT_API
  ${CMAKE_CURRENT_SOURCE_DIR}/scripting/Script.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/scripting/ScriptAPI.cpp
  PARENT_SCOPE
)

SET(TeXworks_SCRIPT_API_H
  ${CMAKE_CURRENT_SOURCE_DIR}/scripting/Script.h
  ${CMAKE_CURRENT_SOURCE_DIR}/scripting/ScriptAPI.h
  ${CMAKE_CURRENT_SOURCE_DIR}/TWSystemCmd.h
  ${CMAKE_CURRENT_SOURCE_DIR}/Settings.h
  PARENT_SCOPE
)


# App Setup
# ---------

# On OS X, set GUI type to an `.app` bundle, gather icon files and add core system
# frameworks to the library link list.
IF ( APPLE )
  SET(GUI_TYPE MACOSX_BUNDLE)

  LIST(APPEND TeXworks_LIBS  "-framework Foundation")

  set(TeXworks_APP_ICONS "${TeXworks_SOURCE_DIR}/res/TeXworks.icns" "${TeXworks_SOURCE_DIR}/res/TeXworks-doc.icns")
  SET_SOURCE_FILES_PROPERTIES( ${TeXworks_APP_ICONS}
    PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
  )
ENDIF ()

# **WARNING:** _Windows is untested_
IF ( WIN32 )
  SET(GUI_TYPE WIN32)
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
  ${TEXWORKS_SRCS} ${TEXWORKS_HDRS}
  ${TEXWORKS_RCS}
  ${TeXworks_APP_ICONS}
)
add_dependencies(TeXworks GitRev)

if (UNIX AND NOT APPLE)
  set_target_properties(TeXworks PROPERTIES
    OUTPUT_NAME texworks
  )
endif ()

target_compile_options(TeXworks PRIVATE ${WARNING_OPTIONS})


# Add translations
target_add_qt_translations(TeXworks
  TS_FILES ${TEXWORKS_TRANS_TS}
  QM_FILES ${QT_TRANS_QM}
  INCLUDEPATH "../src/"
)

get_target_property(lupdatePath Qt${QT_VERSION_MAJOR}::lupdate LOCATION)
get_target_property(_sources TeXworks SOURCES)
add_custom_target(${PROJECT_NAME}_UpdateTranslations COMMAND "${CMAKE_COMMAND}"
  "-DOUTPUTFILE=\"${CMAKE_SOURCE_DIR}/trans/${PROJECT_NAME}_trans.pro\""
  "-DPROJECT_NAME=\"${PROJECT_NAME}\""
  "-DQt_LUPDATE_EXECUTABLE=\"${lupdatePath}\""
  "-DFILES=\"${_sources};${TEXWORKS_UIS};${TEXWORKS_TRANS_TS}\""
  "-DINCLUDEPATH=\"../src/\""
  -P "${PROJECT_SOURCE_DIR}/modules/QtPDF/CMake/Modules/UpdateTranslations.cmake"
  WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
if (NOT TARGET UpdateTranslations)
  add_custom_target(UpdateTranslations)
endif (NOT TARGET UpdateTranslations)
add_dependencies(UpdateTranslations ${PROJECT_NAME}_UpdateTranslations)

IF (UNIX)
  # Required (at least) on OpenBSD to make scripting plugins work
  SET_TARGET_PROPERTIES(TeXworks PROPERTIES LINK_FLAGS -rdynamic)
ENDIF ()

# Specify libraries to be linked to TeXworks.
target_LINK_LIBRARIES(TeXworks QtPDF::qtpdf ${TeXworks_LIBS} )

# Disallow automatic casts from char* to QString (enforcing the use of tr() or
# explicitly specifying the string encoding)
target_compile_definitions(TeXworks PRIVATE QT_NO_CAST_FROM_ASCII QT_NO_CAST_TO_ASCII QT_NO_CAST_FROM_BYTEARRAY)
if (NOT MSVC)
	# Set QT_STRICT_ITERATORS everywhere except for MSVC (QTBUG-78112)
	target_compile_definitions(TeXworks PRIVATE QT_STRICT_ITERATORS)
endif (NOT MSVC)

target_compile_definitions(TeXworks PRIVATE TW_BUILD_ID="${TW_BUILD_ID}")

function (target_add_path_define target name path)
  if (IS_ABSOLUTE ${path})
    target_compile_definitions(${target} PRIVATE ${name}="${path}")
  else ()
    target_compile_definitions(${target} PRIVATE ${name}="${CMAKE_INSTALL_PREFIX}/${path}")
  endif ()
endfunction()

if (UNIX AND NOT APPLE)
  target_add_path_define(TeXworks "TW_HELPPATH" "${TeXworks_HELP_DIR}")
  target_add_path_define(TeXworks "TW_PLUGINPATH" "${TeXworks_PLUGIN_DIR}")
  target_add_path_define(TeXworks "TW_DICPATH" "${TeXworks_DIC_DIR}")
endif ()


# For OS X, generate `Info.plist` and add it to the `.app` bundle.
IF (APPLE)
  SET(VER ${TeXworks_VERSION})
  SET(REV ${TeXworks_GIT_HASH})
  CONFIGURE_FILE(${TeXworks_SOURCE_DIR}/res/TeXworks.plist.in
    TeXworks.plist
    @ONLY
  )

  SET_TARGET_PROPERTIES( TeXworks PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST
      ${CMAKE_CURRENT_BINARY_DIR}/TeXworks.plist
  )
ENDIF ()

IF (UNIX)
    SET_TARGET_PROPERTIES(TeXworks PROPERTIES COMPILE_FLAGS -fPIC)
ENDIF ()

# Installation
# ------------
if (WIN32)
  # On Windows, put everything into the root install directory
  INSTALL(TARGETS TeXworks
    RUNTIME DESTINATION . COMPONENT
  )
else ()
  INSTALL(TARGETS TeXworks
    RUNTIME DESTINATION bin COMPONENT
    BUNDLE DESTINATION . COMPONENT
  )
endif ()

IF ( UNIX AND NOT APPLE )
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR}/512x512/apps)
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks-128.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR}/128x128/apps RENAME TeXworks.png)
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks-64.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR}/64x64/apps RENAME TeXworks.png)
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/images/TeXworks-48.png DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_ICON_DIR}/48x48/apps RENAME TeXworks.png)
  INSTALL(FILES
    ${TeXworks_SOURCE_DIR}/COPYING
    ${TeXworks_SOURCE_DIR}/README.md
    ${TeXworks_SOURCE_DIR}/NEWS
  DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_DOCS_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/texworks.1 DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_MAN_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/texworks.desktop DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_DESKTOP_DIR})
  INSTALL(FILES ${TeXworks_SOURCE_DIR}/res/texworks.appdata.xml DESTINATION ${CMAKE_INSTALL_PREFIX}/${TeXworks_APPDATA_DIR})
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
