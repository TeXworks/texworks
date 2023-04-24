# OS X packaging tasks

# This file is processed by `CONFIGURE_FILE` in `../CMakeLists.txt` which inserts
# values for `@VARIABLE@` declarations. This is done to import values for some
# variables that would otherwise be undefined when CPack is running.
SET(PROJECT_NAME @PROJECT_NAME@)
SET(PROJECT_SOURCE_DIR @PROJECT_SOURCE_DIR@)
SET(PROJECT_BINARY_DIR @PROJECT_BINARY_DIR@)
SET(TeXworks_LIB_DIRS @TeXworks_LIB_DIRS@)
SET(CMAKE_SHARED_LIBRARY_SUFFIX @CMAKE_SHARED_LIBRARY_SUFFIX@)
SET(QT_PLUGINS @QT_PLUGINS@)
SET(QT_VERSION_MAJOR @QT_VERSION_MAJOR@)
set(QT_LIBRARY_DIR @QT_LIBRARY_DIR@)

# TeXworks HTML manual: version, matching hash, and derived variables.
SET(TW_MANUAL_VERSION "20200329154824-18e0761")
SET(TW_MANUAL_ARCHIVE_SHA256 "d74cd729abd1dd4b892f6e6e99334cb3f01aceffb4836d848fc84fcfdf2b8b8e")
SET(TW_MANUAL_BASE "TeXworks-manual-html-${TW_MANUAL_VERSION}")
SET(TW_MANUAL_ARCHIVE "${TW_MANUAL_BASE}.zip")
SET(TW_MANUAL_ARCHIVE_URL "https://github.com/TeXworks/manual/releases/download/2020-03-29/${TW_MANUAL_ARCHIVE}")

# This `IF` statement ensures that the following commands are executed only when
# CPack is running---i.e. when a user executes `make package` but not `make install`
IF ( ${CMAKE_INSTALL_PREFIX} MATCHES .*/_CPack_Packages/.* )

  # Download and install TeXworks manual
  # ------------------------------------
  IF ( NOT EXISTS "${PROJECT_SOURCE_DIR}/${TW_MANUAL_ARCHIVE}" )
    MESSAGE(STATUS "Downloading TeXworks HTML manual from ${TW_MANUAL_ARCHIVE_URL}")
    FILE(DOWNLOAD "${TW_MANUAL_ARCHIVE_URL}"
      "${PROJECT_SOURCE_DIR}/${TW_MANUAL_ARCHIVE}"
      EXPECTED_HASH SHA256=${TW_MANUAL_ARCHIVE_SHA256}
      SHOW_PROGRESS
    )
  ELSE ( )
    MESSAGE(STATUS "Using manual files in '${PROJECT_SOURCE_DIR}/${TW_MANUAL_ARCHIVE}'")
  ENDIF ()

  IF ( NOT EXISTS "${PROJECT_SOURCE_DIR}/${TW_MANUAL_BASE}" )
    FILE(MAKE_DIRECTORY "${PROJECT_SOURCE_DIR}/${TW_MANUAL_BASE}")
    EXECUTE_PROCESS(
      COMMAND unzip "${PROJECT_SOURCE_DIR}/${TW_MANUAL_ARCHIVE}"
      WORKING_DIRECTORY "${PROJECT_SOURCE_DIR}/${TW_MANUAL_BASE}"
    )
  ELSE ( )
    MESSAGE(STATUS "'${PROJECT_SOURCE_DIR}/${TW_MANUAL_BASE}' already present")
  ENDIF ()

  MESSAGE(STATUS "Bundling manual files")
  FILE(INSTALL "${PROJECT_SOURCE_DIR}/${TW_MANUAL_BASE}/TeXworks-manual"
    DESTINATION "${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/texworks-help/"
  )


  # Copy all runtime dependencies and rewrite loader paths
  # ------------------------------------------------------

  # Bring in `DeployQt5` a CMake module taken from the Charm application:
  #
  #   <https://github.com/KDAB/Charm>
  #
  # This module offers the `FIXUP_QT5_BUNDLE` function which wraps
  # `FIXUP_BUNDLE` from CMake's `BundleUtilities` module and extends it with
  # additional Qt5-specific goodies---such as installing Qt5 plugins.
  #
  # `FIXUP_BUNDLE` is a wonderful function that examines an executable, finds
  # all non-system libraries it depends on, copies them into the `.app` bundle
  # and then re-writes the necessary loader paths.
  SET(CMAKE_MODULE_PATH @CMAKE_MODULE_PATH@)
  INCLUDE(DeployQt5)

  # Gather all TeXworks Plugin libraries.
  FILE(GLOB TeXworks_PLUGINS
    ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/*${CMAKE_SHARED_MODULE_SUFFIX})

  # If `BU_CHMOD_BUNDLE_ITEMS` is not set, `install_name_tool` will fail to
  # re-write some loader paths due to insufficiant permissions.
  SET(BU_CHMOD_BUNDLE_ITEMS ON)

  FIXUP_QT5_EXECUTABLE(${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app "${QT_PLUGINS}" "${TeXworks_PLUGINS}" "${TeXworks_LIB_DIRS}")

  # Remove unecessary architectures from universal binaries
  # -------------------------------------------------------

  # Some libraries copied from the OS X system, such as X11 libraries, may
  # contain up to 4 different architectures. Here we will iterate over these
  # libraries and use `lipo` to strip out un-needed architectures.

  # Another useful function from `BundleUtilities`.
  GET_BUNDLE_MAIN_EXECUTABLE(${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app APP_MAIN)

  # We look at the TeXworks binary that was built rather than consulting the
  # value of the `CMAKE_OSX_ARCHITECTURES` because if the user did not set
  # `CMAKE_OSX_ARCHITECTURES`, then the variable will be an empty string and the
  # format of the resulting binary will depend on the versions of OS X and
  # XCode.
  MESSAGE(STATUS "Reducing the size of bundled libraries.")
  MESSAGE(STATUS "Scanning architectures of: ${APP_MAIN}")
  EXECUTE_PROCESS(
    # `lipo -info` returns a list of the form:
    #
    #     <is universal binary?>: <program name>: <list of architectures>
    #
    # Piping this output to `cut -d : -f 3-` allows us to extract just the list
    # of architectures.
    COMMAND lipo -info ${APP_MAIN}
    COMMAND cut -d : -f 3-
    OUTPUT_VARIABLE APP_ARCHS
  )

  # Strip leading and trailing whitespace.
  STRING(STRIP ${APP_ARCHS} APP_ARCHS)
  # Convert spaces to semicolons so CMake will interpret the string as a list.
  STRING(REPLACE " " ";" APP_ARCHS ${APP_ARCHS})

  MESSAGE(STATUS "Will reduce bundled libraries to: ${APP_ARCHS}")

  FOREACH(ARCH IN LISTS APP_ARCHS)
    SET(ARCHS_TO_EXTRACT "${ARCHS_TO_EXTRACT} -extract ${ARCH}")
  ENDFOREACH ()

  # __NOTE:__ This will not process any dylibs from Frameworks copied by
  # `FIXUP_BUNDLE`, hence it may not touch any of the Qt libraries. Something to
  # fix in the future.
  FILE(GLOB BUNDLED_DYLIBS
    ${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/MacOS/*${CMAKE_SHARED_LIBRARY_SUFFIX})

  FOREACH(DYLIB IN LISTS BUNDLED_DYLIBS)
    # `lipo` prints error messages when attempting to extract from a file that
    # is not a universal (fat) binary. Avoid this by checking first.
    EXECUTE_PROCESS(
      COMMAND lipo -info ${DYLIB}
      COMMAND cut -d : -f 1
      COMMAND grep -q "Non-fat file"
      RESULT_VARIABLE DYLIB_IS_FAT
    )
    IF(NOT ${DYLIB_IS_FAT} EQUAL 0)
      MESSAGE(STATUS "Processing fat library: ${DYLIB}")
      # `lipo` is very very anal about how arguments get passed to it. So we
      # execute through bash to side-step the issue.
      EXECUTE_PROCESS(
        COMMAND bash -c "lipo ${ARCHS_TO_EXTRACT} ${DYLIB} -output ${DYLIB}"
      )
    ELSE()
      MESSAGE(STATUS "Skipping non-fat library: ${DYLIB}")
    ENDIF()
  ENDFOREACH ()

  MESSAGE(STATUS "Finished stripping architectures from bundled libraries.")

ENDIF ()

