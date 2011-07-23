# Build script for the Python Plugin
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TeXworks_SOURCE_DIR}/src
  ${QT_INCLUDE_DIR}
  ${PYTHON_INCLUDE_DIRS}
)

LINK_DIRECTORIES(${QT_LIBRARY_DIR})

# Re-moccing headers from the main source directory that have already been
# mocced during the build of the main program seems redundant, but I'm not wise
# enough in the was of Qt and CMake to figure out an alternative.
QT4_WRAP_CPP(PYTHON_PLUGIN_MOC ${TeXworks_SCRIPT_API_H} TWPythonPlugin.h)

ADD_LIBRARY(TWPythonPlugin SHARED
  TWPythonPlugin.cpp
  ${TeXworks_SCRIPT_API}
  ${PYTHON_PLUGIN_MOC}
)
TARGET_LINK_LIBRARIES(TWPythonPlugin ${QT_LIBRARIES} ${PYTHON_LIBRARIES})

INSTALL(TARGETS TWPythonPlugin
  LIBRARY DESTINATION ${TeXworks_PLUGIN_DIR}
)
