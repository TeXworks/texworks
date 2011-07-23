# OS X packaging tasks
INSTALL( CODE
  "
  # Only execute these steps if CPack is building a package for distribution
  IF( \${CMAKE_INSTALL_PREFIX} MATCHES .*/_CPack_Packages/.* )

    # Download and install Poppler data.
    IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/poppler-data-0.4.4.tar.gz )
      MESSAGE(STATUS \"Downloading Poppler data files\")
      FILE(DOWNLOAD \"http://poppler.freedesktop.org/poppler-data-0.4.4.tar.gz\"
        ${TeXworks_SOURCE_DIR}/poppler-data-0.4.4.tar.gz
        EXPECTED_MD5 f3a1afa9218386b50ffd262c00b35b31
        SHOW_PROGRESS
      )
    ENDIF ()

    IF ( NOT EXISTS ${TeXworks_SOURCE_DIR}/poppler-data-0.4.4 )
      EXECUTE_PROCESS(COMMAND tar xzf ${TeXworks_SOURCE_DIR}/poppler-data-0.4.4.tar.gz
        WORKING_DIRECTORY ${TeXworks_SOURCE_DIR}
      )
    ENDIF ()

    INSTALL(DIRECTORY ${TeXworks_SOURCE_DIR}/poppler-data-0.4.4/
      DESTINATION ${PROJECT_NAME}.app/Contents/poppler-data
      PATTERN \"CMakeLists.txt\" EXCLUDE
      PATTERN \"Makefile\" EXCLUDE
    )

    # FIXUP_BUNDLE is a wonderful function provided by CMake's BundleUtilities
    # that examines an executable, finds all non-system libraries it depends
    # on, copies them into the .app and then re-writes the necessary loader
    # paths.
    INCLUDE(BundleUtilities)

    # Gather all Plugin libraries
    file(GLOB  QTPLUGINS
      \"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app/Contents/PlugIns/*${CMAKE_SHARED_LIBRARY_SUFFIX}\")

    # If BU_CHMOD_BUNDLE_ITEMS is not set, install_name_tool will fail to
    # re-write some loader paths due to insufficiant permissions.
    SET(BU_CHMOD_BUNDLE_ITEMS ON)
    FIXUP_BUNDLE(\"\${CMAKE_INSTALL_PREFIX}/${PROJECT_NAME}.app\" \"\${QTPLUGINS}\" ${TeXworks_LIB_DIRS})
  ENDIF ()
  "
)

