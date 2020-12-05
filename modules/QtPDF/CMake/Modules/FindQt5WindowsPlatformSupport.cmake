# For static Windows builds, we also need to pull in the Qt5 Platform
# support library, which is not exposed to CMake properly, unfortunately

# This file defines Qt5::WindowsPlatformSupport and sets
# QT_PLATFORM_LIBRARIES = Qt5::WindowsPlatformSupport

if (QT_PLATFORM_LIBRARIES)
  return()
endif ()

get_target_property(QT_LIB_DIR Qt5::Widgets LOCATION)
get_filename_component(QT_LIB_DIR "${QT_LIB_DIR}" PATH)

find_library(Qt5QWindows_LIBRARIES qwindows
  HINTS "${QT_LIB_DIR}/../plugins/platforms"
)

# Starting with Qt 5.8, Qt5PlatformSupport was modularized
if (Qt5Widgets_VERSION VERSION_LESS 5.8)
  find_library(Qt5Platform_LIBRARIES Qt5PlatformSupport
    HINTS "${QT_LIB_DIR}"
  )
  set(QT_PLATFORM_LIBRARIES ${Qt5Platform_LIBRARIES})
else()
  find_library(Qt5FontDatabaseSupport_LIBRARIES Qt5FontDatabaseSupport
    HINTS "${QT_LIB_DIR}"
  )
  find_library(Qt5EventDispatcherSupport_LIBRARIES Qt5EventDispatcherSupport
    HINTS "${QT_LIB_DIR}"
  )
  find_library(Qt5ThemeSupport_LIBRARIES Qt5ThemeSupport
    HINTS "${QT_LIB_DIR}"
  )
  find_library(Qt5AccessibilitySupport_LIBRARIES Qt5AccessibilitySupport
    HINTS "${QT_LIB_DIR}"
  )
  set(QT_PLATFORM_LIBRARIES ${Qt5FontDatabaseSupport_LIBRARIES} ${Qt5EventDispatcherSupport_LIBRARIES} ${Qt5ThemeSupport_LIBRARIES} ${Qt5AccessibilitySupport_LIBRARIES})
endif()
# Starting with Qt 5.11, WindowsUIAutomation was added
if (NOT Qt5Widgets_VERSION VERSION_LESS 5.11)
  find_library(Qt5WindowsUIAutomation_LIBRARIES Qt5WindowsUIAutomationSupport
    HINTS "${QT_LIB_DIR}"
  )
  list(APPEND QT_PLATFORM_LIBRARIES ${Qt5WindowsUIAutomation_LIBRARIES})
endif()

if (PLATFORM_DEPENDENCIES)
  list(APPEND QT_PLATFORM_LIBRARIES ${PLATFORM_DEPENDENCIES})
endif ()

if (NOT TARGET Qt5::WindowsPlatformSupport)
  add_library(Qt5::WindowsPlatformSupport UNKNOWN IMPORTED)
  set_target_properties(Qt5::WindowsPlatformSupport PROPERTIES
    IMPORTED_LOCATION "${Qt5QWindows_LIBRARIES}"
    INTERFACE_LINK_LIBRARIES "${QT_PLATFORM_LIBRARIES}"
    INTERFACE_COMPILE_DEFINITIONS "STATIC_QT5"
  )

  set(QT_PLATFORM_LIBRARIES Qt5::WindowsPlatformSupport)
endif ()
