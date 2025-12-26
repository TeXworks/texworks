
SET(MODULE_INCLUDE_DIRS)

SET(QTPDF_LIB_TYPE STATIC)
SET(QTPDF_VIEWER OFF)
ADD_SUBDIRECTORY(QtPDF)

# We DO NOT include the synctex subdirectory here as that is handled entirely by
# FindSynctex.cmake

add_library(scintilla
	scintilla/qt/ScintillaEdit/ScintillaEdit.cpp
	scintilla/qt/ScintillaEdit/ScintillaDocument.cpp
	scintilla/qt/ScintillaEditBase/PlatQt.cpp
	scintilla/qt/ScintillaEditBase/ScintillaQt.cpp
	scintilla/qt/ScintillaEditBase/ScintillaEditBase.cpp
	scintilla/src/XPM.cxx
	scintilla/src/ViewStyle.cxx
	scintilla/src/UndoHistory.cxx
	scintilla/src/UniqueString.cxx
	scintilla/src/UniConversion.cxx
	scintilla/src/Style.cxx
	scintilla/src/Selection.cxx
	scintilla/src/ScintillaBase.cxx
	scintilla/src/RunStyles.cxx
	scintilla/src/RESearch.cxx
	scintilla/src/PositionCache.cxx
	scintilla/src/PerLine.cxx
	scintilla/src/MarginView.cxx
	scintilla/src/LineMarker.cxx
	scintilla/src/KeyMap.cxx
	scintilla/src/Indicator.cxx
	scintilla/src/Geometry.cxx
	scintilla/src/EditView.cxx
	scintilla/src/Editor.cxx
	scintilla/src/EditModel.cxx
	scintilla/src/Document.cxx
	scintilla/src/Decoration.cxx
	scintilla/src/DBCS.cxx
	scintilla/src/ContractionState.cxx
	scintilla/src/CharClassify.cxx
	scintilla/src/CharacterType.cxx
	scintilla/src/CharacterCategoryMap.cxx
	scintilla/src/ChangeHistory.cxx
	scintilla/src/CellBuffer.cxx
	scintilla/src/CaseFolder.cxx
	scintilla/src/CaseConvert.cxx
	scintilla/src/CallTip.cxx
	scintilla/src/AutoComplete.cxx
	scintilla/qt/ScintillaEdit/ScintillaEdit.h
	scintilla/qt/ScintillaEdit/ScintillaDocument.h
	scintilla/qt/ScintillaEditBase/ScintillaEditBase.h
	scintilla/qt/ScintillaEditBase/ScintillaQt.h
)
target_compile_definitions(scintilla PUBLIC SCINTILLA_QT=1 MAKING_LIBRARY=1)
target_include_directories(scintilla PUBLIC scintilla/qt/ScintillaEdit scintilla/qt/ScintillaEditBase scintilla/src scintilla/include)
target_link_libraries(scintilla Qt${QT_VERSION_MAJOR}::Widgets)
if (${QT_VERSION_MAJOR} LESS 6)
	target_link_libraries(scintilla Qt${QT_VERSION_MAJOR}::Widgets)
else ()
	target_link_libraries(scintilla Qt${QT_VERSION_MAJOR}::Widgets Qt${QT_VERSION_MAJOR}::Core5Compat)
endif ()
