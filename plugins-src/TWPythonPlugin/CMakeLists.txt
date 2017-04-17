# Build Python Plugin
# ===================

INCLUDE_DIRECTORIES(
  ${TeXworks_SOURCE_DIR}/src
  ${PYTHON_INCLUDE_DIRS}
)

LINK_DIRECTORIES(${QT_LIBRARY_DIR})

SET(TWPYTHONPLUGIN_LIB_TYPE MODULE)
IF (NOT ${BUILD_SHARED_PLUGINS})
  SET(TWPYTHONPLUGIN_LIB_TYPE STATIC)
ENDIF()

ADD_LIBRARY(TWPythonPlugin ${TWPYTHONPLUGIN_LIB_TYPE}
  TWPythonPlugin.cpp
  ${PYTHON_PLUGIN_MOC}
)

# Specify link libraries even if the plugin is built statically so all the
# interface properties of the Qt targets (include directories, lib directories,
# etc.) are available
TARGET_LINK_LIBRARIES(TWPythonPlugin ${QT_LIBRARIES} ${PYTHON_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
IF (${BUILD_SHARED_PLUGINS})
  INSTALL(TARGETS TWPythonPlugin
    LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
  )
ENDIF()
