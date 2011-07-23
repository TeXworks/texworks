# Collect source files and store them in CMake variables
FILE(GLOB TEXWORKS_SRCS       "*.cpp" "*.c")
FILE(GLOB TEXWORKS_MOC_HDRS   "*.h")
FILE(GLOB TEXWORKS_UIS        "*.ui")
FILE(GLOB TEXWORKS_RCS        "${TeXworks_SOURCE_DIR}/res/*.qrc")
FILE(GLOB TEXWORKS_TRANS      "${TeXworks_SOURCE_DIR}/trans/*.ts")


# Determine which type of app to make.
SET(GUI_TYPE)
SET(QTCONF_DIR bin)

IF ( APPLE )
  SET(GUI_TYPE MACOSX_BUNDLE)
  SET(MACOSX_BUNDLE_INFO_PLIST ${TeXworks_SOURCE_DIR}/TeXworks.plist)
  SET(MACOSX_BUNDLE_ICON_FILE ${TeXworks_SOURCE_DIR}/TeXworks.icns)

  SET(TeXworks_LIBS ${TeXworks_LIBS} "-framework CoreServices")

  FILE(GLOB TeXworks_APP_ICONS ${TeXworks_SOURCE_DIR}/*.icns)
  SET_SOURCE_FILES_PROPERTIES( ${TeXworks_APP_ICONS}
    PROPERTIES
      MACOSX_PACKAGE_LOCATION Resources
  )

  SET(QTCONF_DIR ${PROJECT_NAME}.app/Contents/Resources)
ENDIF ()

IF ( WIN32 )
  # Untested
  SET(GUI_TYPE WIN32)
ENDIF ()


# Make sure the compiler can find include files.
INCLUDE_DIRECTORIES(
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${TeXworks_INCLUDE_DIRS}
)

# Make sure the compiler can find library files.
LINK_DIRECTORIES( ${TeXworks_LIB_DIRS} )

# Poppler XPDF Support?
IF (POPPLER_HAS_XPDF)
  INCLUDE_DIRECTORIES(${POPPLER_XPDF_INCLUDE_DIR})
  ADD_DEFINITIONS( -DHAVE_POPPLER_XPDF_HEADERS )
ENDIF ()

IF ( APPLE )
  # For some reason, QT4_WRAP_CPP may neglect to define Q_WS_MAC when running
  # Qt tools such as `moc`. This causes unwanted D-Bus dependencies to slip in.
  ADD_DEFINITIONS( -DQ_WS_MAC )
ENDIF ()

# Run moc on header files.
QT4_WRAP_CPP(TEXWORKS_GEN_MOC ${TEXWORKS_MOC_HDRS})
# Run uic on user interface files.
QT4_WRAP_UI(TEXWORKS_GEN_UI ${TEXWORKS_UIS})
# Run rcc on reasource files.
QT4_ADD_RESOURCES(TEXWORKS_GEN_RCS ${TEXWORKS_RCS})
# Prep language translations.
QT4_ADD_TRANSLATION(TEXWORKS_GEN_TRANS ${TEXWORKS_TRANS})

IF (APPLE)
  REMOVE_DEFINITIONS( -DQ_WS_MAC )
ENDIF ()

# Build the executible.
ADD_EXECUTABLE( TeXworks ${GUI_TYPE}
  ${TEXWORKS_SRCS}
  ${TEXWORKS_GEN_MOC} ${TEXWORKS_GEN_UI} ${TEXWORKS_GEN_RCS} ${TEXWORKS_GEN_TRANS}
  ${TeXworks_APP_ICONS}
)

# Specify libraries for linking.
TARGET_LINK_LIBRARIES( TeXworks ${TeXworks_LIBS} )

INSTALL( TARGETS TeXworks
  RUNTIME DESTINATION bin
  BUNDLE DESTINATION .
)

# Write a blank qt.conf file as all hell will break loose if one isn't there.
# Once support for scripting plugins is added, this may need to be done
# elsewhere.
#
# Protip: If there are wierd errors like "Qt libraries loaded twice" or "looking
# for a tablet device", then something may have gone pear shaped with qt.conf
INSTALL( CODE
  "FILE(WRITE \"\${CMAKE_INSTALL_PREFIX}/${QTCONF_DIR}/qt.conf\" \"\")"
)

