
SET(MODULE_INCLUDE_DIRS)

SET(QTPDF_LIB_TYPE STATIC)
SET(QTPDF_VIEWER OFF)
ADD_SUBDIRECTORY(QtPDF)

# We DO NOT include the synctex subdirectory here as that is handled entirely by
# FindSynctex.cmake
