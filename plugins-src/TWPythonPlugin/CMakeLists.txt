# Build Python Plugin
# ===================

SET(TWPYTHONPLUGIN_LIB_TYPE MODULE)
IF (NOT ${BUILD_SHARED_PLUGINS})
  SET(TWPYTHONPLUGIN_LIB_TYPE STATIC)
ENDIF()

if (BUILD_SHARED_PLUGINS)
  # For shared builds, we pull in scripting/Script.cpp so all references can be resolved
  set(TWSCRIPT_SRC "../../src/scripting/Script.cpp")
else ()
  # For static builds, scripting/Script.cpp is already included in the main app, so we
  # don't pull it in here to avoid multiple definitions of the Qt metaobject
  set(TWSCRIPT_SRC "")
endif ()

ADD_LIBRARY(TWPythonPlugin ${TWPYTHONPLUGIN_LIB_TYPE}
  PythonScriptInterface.cpp
  PythonScript.cpp
  ${TWSCRIPT_SRC}
  ${PYTHON_PLUGIN_MOC}
)
target_compile_options(TWPythonPlugin PRIVATE ${WARNING_OPTIONS})
if (NOT MSVC)
	# Don't warn about old-style casts as the Python headers are littered with
	# macros (which are handled as if copied into our code) using those
	target_compile_options(TWPythonPlugin PRIVATE -Wno-old-style-cast)
endif ()

target_include_directories(TWPythonPlugin SYSTEM PRIVATE ${PYTHON_INCLUDE_DIRS})
target_include_directories(TWPythonPlugin PRIVATE ${TeXworks_SOURCE_DIR}/src)

# Specify link libraries even if the plugin is built statically so all the
# interface properties of the Qt targets (include directories, lib directories,
# etc.) are available
TARGET_LINK_LIBRARIES(TWPythonPlugin ${QT_LIBRARIES} ${PYTHON_LIBRARIES} ${TEXWORKS_ADDITIONAL_LIBS})
IF (${BUILD_SHARED_PLUGINS})
  INSTALL(TARGETS TWPythonPlugin
    LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
  )
ENDIF()
