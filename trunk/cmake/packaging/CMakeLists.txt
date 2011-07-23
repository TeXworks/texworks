IF ( APPLE )

  # Create a CMake script containing packaging tasks specific to Apple. All
  # variables of the form @VARIABLE@ will be replaced, while variables of the
  # form ${VARIABLE} will be left untouched.
  CONFIGURE_FILE(
    ${PROJECT_SOURCE_DIR}/cmake/packaging/mac/MacPackagingTasks.cmake.in
    ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake
    @ONLY
  )

ELSE ()

  # Create a dummy script.
  FILE(WRITE ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake "")

ENDIF ()

# Execute the packaging tasks. INSTALL(SCRIPT ...) is used so that these tasks
# occur during the "make install" phase rather than the "make" phase or the
# "cmake" phase of building the project.
INSTALL(SCRIPT ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake)

