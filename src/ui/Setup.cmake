#[===============================================[
This is part of TWX build and test system.
https://github.com/TeXworks/texworks

This is `<...>/src/ui/Setup.cmake`

Usage from an external build directory:
```
include ( SrcTwxUISetup )
```
Output:
* `TwxUI_SOURCES`, a `;` separated list of full paths
* `TwxUI_HEADERS`, a `;` separated list of full paths

Includes `TwxConfigureFilePaths`.
#]===============================================]

set ( TwxUI_SOURCES )
set ( TwxUI_HEADERS )
set ( TwxUI_UIS )

# if ( NOT TWX_IS_BASED )
#   message ( FATAL_ERROR "TwxBase is not included")
# endif ()

list (
	APPEND TwxUI_SOURCES
	"${CMAKE_CURRENT_LIST_DIR}/ClickableLabel.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ClosableTabWidget.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ColorButton.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ConsoleWidget.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/LineNumberWidget.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ListSelectDialog.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/RemoveAuxFilesDialog.cpp"
	"${CMAKE_CURRENT_LIST_DIR}/ScreenCalibrationWidget.cpp"
)

list (
	APPEND TwxUI_HEADERS
	"${CMAKE_CURRENT_LIST_DIR}/ClickableLabel.h"
	"${CMAKE_CURRENT_LIST_DIR}/ClosableTabWidget.h"
	"${CMAKE_CURRENT_LIST_DIR}/ColorButton.h"
	"${CMAKE_CURRENT_LIST_DIR}/ConsoleWidget.h"
	"${CMAKE_CURRENT_LIST_DIR}/LineNumberWidget.h"
	"${CMAKE_CURRENT_LIST_DIR}/ListSelectDialog.h"
	"${CMAKE_CURRENT_LIST_DIR}/RemoveAuxFilesDialog.h"
	"${CMAKE_CURRENT_LIST_DIR}/ScreenCalibrationWidget.h"
)

list (
	APPEND TwxUI_UIS
	"${CMAKE_CURRENT_LIST_DIR}/ListSelectDialog.ui"
)
