# OS X packaging tasks

# FIXUP_BUNDLE is a wonderful function provided by CMake's BundleUtilities that
# examines an executable, finds all non-system libraries it depends on, copies
# them into the .app and then re-writes the necessary loader paths.
INSTALL( CODE
  "
  INCLUDE(BundleUtilities)

  # Gather all Plugin libraries
  file(GLOB  QTPLUGINS
    \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/plugins/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")

  # If BU_CHMOD_BUNDLE_ITEMS is not set, install_name_tool will fail to
  # re-write some loader paths due to insufficiant permissions.
  SET(BU_CHMOD_BUNDLE_ITEMS ON)
  FIXUP_BUNDLE(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\" \"\${QTPLUGINS}\" ${TeXworks_LIB_DIRS})
  "
)

