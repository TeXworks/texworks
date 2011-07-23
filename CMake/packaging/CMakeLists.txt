# Packaging Tasks
# ===============

# Create a CMake script containing packaging tasks specific to Apple. All
# variables of the form `@VARIABLE@` will be replaced, while variables of the
# form `${VARIABLE}` will be left untouched.
IF ( APPLE )
  CONFIGURE_FILE(
    ${PROJECT_SOURCE_DIR}/cmake/packaging/mac/MacPackagingTasks.cmake.in
    ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake
    @ONLY
  )
ELSE ()
  # Create a dummy script for other platforms.
  FILE(WRITE ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake "")
ENDIF ()

# Execute the packaging tasks. `INSTALL(SCRIPT ...)` is used so that these tasks
# executing during `make install` rather than `make` or `cmake`.
INSTALL(SCRIPT ${PROJECT_BINARY_DIR}/CPackPackagingTasks.cmake)

