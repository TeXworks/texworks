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


TODO
====
### Required
 - ~~GotoPage should not center on page~~ __DONE__
 - zoom/hand tools
 - ctrl+mousewheel should zoom
 - maquee zoom (requested by Philip Taylor)
 - context menu
 - Flesh out link support
 - Add some loading indicators so that users know something is happening in the
   background when pages are blank or blurry
 - scrolling beyond edge at single page mode (mouse wheel/up/down)
 - disallow keyboard events in document view while magnifier is shown (otherwise
   one could scroll the canvas without properly adjusting the magnifier)
 - SyncTeX with signals and slots
 - ~~fit to width/page/~~ __DONE__
 - Highlighting! (to show syncing destination, search results, etc.)
 - ~~PgDn should scroll one viewport height~~ __DONE__
 - ~~draw frame around magnifying glass (requested by Reinhard Kotucha)~~ __DONE__
 - make the magnifying glass a top-level window (so it can extend outside the
   main view's window boundaries) (requested by Reinhard Kotucha)
 - Comment on how `pageScale` works and is used
 - Control memory usage through caching and zoom throttling

### Wishlist
 - annotations (popup window)
 - rulers (in main window, and attached to magnifying glass)
 - ~~possibility to abort render requests when page moves out of view (is there a
   way to avoid going through all pages at each scroll event?)~~ __DONE__
 - Split view???
 - Improve handling of several concurrent versions (magnifications) of the same
   page (currently: "normal" and "magnified" versions); simplify code, remove
   redundancy, etc.
