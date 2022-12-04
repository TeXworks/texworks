# Build Lua Plugin
# ================

SET(TWLUAPLUGIN_LIB_TYPE MODULE)
IF (NOT ${BUILD_SHARED_PLUGINS})
  SET(TWLUAPLUGIN_LIB_TYPE STATIC)
ENDIF()

if (BUILD_SHARED_PLUGINS)
  # For shared builds, we pull in scripting/Script.cpp so all references can be resolved
  # This seems to be needed particularly when building dlls with MinGW on
  # Windows (as is done in the Appveyor CI build) as the dll linking stage
  # requires all symbols to be resolved
  # NB: This can lead to spurious "one definition rule (odr) violation" messages
  # with some sanitizers
  # TODO: Figure out of setting ENABLE_EXPORTS=TRUE on the TeXworks target and
  # subsequent linking to the TeXworks target can help "resolve" symbols by
  # telling the linker they can be found in the host program loading the dll
  set(TWSCRIPT_SRC "../../src/scripting/Script.cpp")
else ()
  # For static builds, scripting/Script.cpp is already included in the main app, so we
  # don't pull it in here to avoid multiple definitions of the Qt metaobject
  set(TWSCRIPT_SRC "")
endif ()

ADD_LIBRARY(TWLuaPlugin ${TWLUAPLUGIN_LIB_TYPE}
  LuaScriptInterface.cpp
  LuaScript.cpp
  ${TWSCRIPT_SRC}
  ${LUA_PLUGIN_MOC}
)
target_compile_options(TWLuaPlugin PRIVATE ${WARNING_OPTIONS})
# Disallow automatic casts from char* to QString (enforcing the use of tr() or
# explicitly specifying the string encoding)
target_compile_definitions(TWLuaPlugin PRIVATE -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY)

target_include_directories(TWLuaPlugin SYSTEM PRIVATE ${LUA_INCLUDE_DIR})
target_include_directories(TWLuaPlugin PRIVATE ${TeXworks_SOURCE_DIR}/src)

# Specify link libraries even if the plugin is built statically so all the
# interface properties of the Qt targets (include directories, lib directories,
# etc.) are available
TARGET_LINK_LIBRARIES(TWLuaPlugin ${QT_LIBRARIES} ${LUA_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
IF (${BUILD_SHARED_PLUGINS})
  INSTALL(TARGETS TWLuaPlugin
    LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
  )
ENDIF()
