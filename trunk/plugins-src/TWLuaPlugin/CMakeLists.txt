# Build Lua Plugin
# ================

INCLUDE_DIRECTORIES(
  ${TeXworks_SOURCE_DIR}/src
  ${QT_INCLUDE_DIR}
  ${LUA_INCLUDE_DIR}
)

LINK_DIRECTORIES(${QT_LIBRARY_DIR})

# **NOTE:** _Re-moccing headers from the main source directory that have
# already been mocced during the build of the main program seems redundant, but
# I'm not wise enough in the ways of Qt and CMake to figure out a good
# alternative._
IF (${QT_VERSION_MAJOR} EQUAL 5)
  QT5_WRAP_CPP(LUA_PLUGIN_MOC ${TeXworks_SCRIPT_API_H} TWLuaPlugin.h)
ELSE()
  QT4_WRAP_CPP(LUA_PLUGIN_MOC ${TeXworks_SCRIPT_API_H} TWLuaPlugin.h)
ENDIF()

ADD_LIBRARY(TWLuaPlugin MODULE
  TWLuaPlugin.cpp
  ${TeXworks_SCRIPT_API}
  ${LUA_PLUGIN_MOC}
)
TARGET_LINK_LIBRARIES(TWLuaPlugin ${QT_LIBRARIES} ${LUA_LIBRARIES})

INSTALL(TARGETS TWLuaPlugin
  LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
)
