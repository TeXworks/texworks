# Build Lua Plugin
# ================

INCLUDE_DIRECTORIES(
  ${TeXworks_SOURCE_DIR}/src
  ${LUA_INCLUDE_DIR}
)

LINK_DIRECTORIES(${QT_LIBRARY_DIR})

SET(TWLUAPLUGIN_LIB_TYPE MODULE)
IF (NOT ${BUILD_SHARED_PLUGINS})
  SET(TWLUAPLUGIN_LIB_TYPE STATIC)
ENDIF()

ADD_LIBRARY(TWLuaPlugin ${TWLUAPLUGIN_LIB_TYPE}
  TWLuaPlugin.cpp
  ${LUA_PLUGIN_MOC}
)

# Specify link libraries even if the plugin is built statically so all the
# interface properties of the Qt targets (include directories, lib directories,
# etc.) are available
TARGET_LINK_LIBRARIES(TWLuaPlugin ${QT_LIBRARIES} ${LUA_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
IF (${BUILD_SHARED_PLUGINS})
  INSTALL(TARGETS TWLuaPlugin
    LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
  )
ENDIF()
