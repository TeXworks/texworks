# Packaging Tasks
# ===============

# Create a CMake script containing packaging tasks specific to Apple. All
# variables of the form `@VARIABLE@` will be replaced, while variables of the
# form `${VARIABLE}` will be left untouched.
IF ( APPLE )
  CONFIGURE_FILE(
    ${PROJECT_SOURCE_DIR}/CMake/packaging/mac/MacPackagingTasks.cmake.in
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

set(POPPLERDATA_URL https://poppler.freedesktop.org/poppler-data-0.4.10.tar.gz)
set(POPPLERDATA_SHA256 6e2fcef66ec8c44625f94292ccf8af9f1d918b410d5aa69c274ce67387967b30)

set(MANUAL_HTML_URL https://github.com/TeXworks/manual/releases/download/2021-03-08/TeXworks-manual-html-20210308173322-7d24168.zip)
set(MANUAL_HTML_SHA256 b3fea63be210367f24d2d0a70c8e272f107edc4f53e241ec19c61e63eee99569)

set(CPACK_PROJECT_CONFIG_FILE "${CMAKE_SOURCE_DIR}/CMake/packaging/CPackProjectConfig.txt" PARENT_SCOPE)

# Set custom CPack variables
set(CPACK_GIT_HASH "${TeXworks_GIT_HASH}" PARENT_SCOPE)

if (WIN32)
	set(CPACK_TARGET_OS "win" PARENT_SCOPE)
	set(CPACK_GENERATOR "ZIP" PARENT_SCOPE)
	set(CPACK_STRIP_FILES ON PARENT_SCOPE)
	set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY OFF PARENT_SCOPE)
	include(ExternalProject)
	ExternalProject_Add(poppler-data
		URL ${POPPLERDATA_URL}
		URL_HASH SHA256=${POPPLERDATA_SHA256}
		CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_BINARY_DIR}
	)
	file(COPY "${CMAKE_SOURCE_DIR}/win32/fonts" DESTINATION "${CMAKE_BINARY_DIR}/share")

	install(DIRECTORY ${CMAKE_BINARY_DIR}/share
		DESTINATION "."
		PATTERN pkgconfig EXCLUDE)
	install(FILES "${CMAKE_SOURCE_DIR}/COPYING" DESTINATION ".")
	install(FILES "${CMAKE_SOURCE_DIR}/win32/README.win" DESTINATION "." RENAME "README.txt")

	ExternalProject_Add(texworks-help
		URL ${MANUAL_HTML_URL}
		URL_HASH SHA256=${MANUAL_HTML_SHA256}
		BUILD_IN_SOURCE TRUE
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory . ${CMAKE_BINARY_DIR}/texworks-help/TeXworks-manual
	)
	install(DIRECTORY ${CMAKE_BINARY_DIR}/texworks-help DESTINATION ".")
elseif (APPLE)
	set(CPACK_TARGET_OS "macos" PARENT_SCOPE)
	include(ExternalProject)
	ExternalProject_Add(poppler-data
		URL ${POPPLERDATA_URL}
		URL_HASH SHA256=${POPPLERDATA_SHA256}
		CMAKE_CACHE_ARGS -DCMAKE_INSTALL_PREFIX:STRING=${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/
	)
	file(REMOVE_RECURSE ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/share/pkgconfig)

	ExternalProject_Add(texworks-help
		URL ${MANUAL_HTML_URL}
		URL_HASH SHA256=${MANUAL_HTML_SHA256}
		BUILD_IN_SOURCE TRUE
		CONFIGURE_COMMAND ""
		BUILD_COMMAND ""
		INSTALL_COMMAND ${CMAKE_COMMAND} -E copy_directory . ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.app/Contents/texworks-help/TeXworks-manual
	)
endif ()
