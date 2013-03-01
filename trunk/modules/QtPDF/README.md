Experimental Multi-Page PDF Viewer
==================================

This branch contains an work-in-progress PDF viewer that is intended to replace
the `PDFWidget` class in `src/PDFDocument.cpp`. Only the files in `src` are
intended to be merged into the TeXworks tree---everything else is just a test
app that streamlines the prototyping process.

The test app is called `pdf_viewer` and upon launching, it uses the classes
contained in the `src` folder to display a copy of the PGF manual. The PGF
manual was chosen as a test case because:

  - It was produced by TeX.
  - It is extensively cross-referenced with hyperlinks.
  - It is quite large and has many complex layouts. This is a good stress test
    of rendering performance.


Things Needed to Build
======================

A simple Makefile is used to build the test app called `pdf_viewer`. The
following components are required:

  - pkg-config
  - qt
  - freetype2
  - poppler and poppler-qt4

