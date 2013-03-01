Experimental Multi-Page PDF Viewer
==================================

This branch contains an work-in-progress PDF viewer that is intended to replace
the `PDFWidget` class in `src/PDFDocument.cpp`. Only the files in `src` are
intended to be merged into the TeXworks tree---everything else is part of test
apps that streamline the prototyping process.

Upon launching, the test app uses the classes contained in the `src` folder to
display a copy of the PGF manual. The PGF manual was chosen as a test case
because:

  - It was produced by TeX.
  - It is extensively cross-referenced with hyperlinks.
  - It is quite large and has many complex layouts. This is a good stress test
    of rendering performance.


Things Needed to Build
======================

CMake is used to build a library containing the rendering code and test apps
that use different backends.

The following libraries are required by all targets:

  - qt
  - zlib
  - freetype2

For the Poppler backend, the following components are needed:

  - poppler and poppler-qt4

For the MuPDF backend, the following components are needed:

  - jpeg
  - openjpeg
  - jbig2dec
  - mupdf

By default, CMake will look for Poppler and produce the `poppler_viewer`
executable. To enable MuPDF and the `mupdf_viewer`, add `-DWITH_MUDPF=YES` to
the options passed to `cmake`.


Building on Windows
===================

Windows builds can be accomplished using MinGW, MSYS and CMake. Assuming Qt is
installed and the required dependencies have been built and installed to
`/c/opt/texworks`, the following steps can be invoked from a MSYS shell to
build the project:

    mkdir build
    cd build
    cmake .. -G "MSYS Makefiles" -DCMAKE_PREFIX_PATH=/c/opt/texworks

    make


TODO
====
### Required
 - Fix FIXMEs
 - context menu
 - Highlighting! (to show syncing destination, search results, etc.)
 - possibility to abort render requests when page moves out of view (is there a
   way to avoid going through all pages at each scroll event?)
 - Possibly simplify page processing request generation. Rational: Right now,
   requestLoadLinks, requestRenderPage, and addPageProcessingRequest are all
   called from the main thread, if I understood threading correctly.
   Consequently, the page processing request object should live in the main
   thread as well (but is accessed only from the worked thread). If that is
   correct, all the moving of objects is superfluous as well.
 - Implement text search for PDF files.
 - Port scripting-related functions from Tw (if any)
 - Handling of encrypted/password protected files (this needs to be handled
   during loading; accessing some functions (e.g., for rendering) in a locked
   document crashes the program)
 - Add a PDFDocumentWidget, derived from PDFDocumentView, which encapsulates the
   scene and provides, e.g., a load() function for ease of use (similar to what
   Qt does, see QTreeView vs. QTreeWidget)
 - Possibility to reload current file (e.g., if it changed on disk); note that
   this requires some reconsideration of memory management, as Page object
   could change, which would have to be propagated upwards to every object
   holding a pointer to them (e.g., PDFPageGraphicsItem)
 - Make program translatable; this probably entails creating a (dummy) Qt .pro
   file so we can use the normal Qt linguist approach
 - Clicking on an item in the ToC that lies after the current viewport position
   moves the view such that a small band of the page in question is visible at
   the bottom of the screen---but we should see that page fill the entire screen
   Note: this only applies of no viewport setting is associated with the toc item
 - ~~Icons (add icons for page modes, make marquee zoom icon consistent with
   other icons)~~ __DONE__
 - ~~Turn PDFViewer into a more general test case (loading of arbitrary files,
   etc.)~~ __DONE__
 - ~~Program segfaults if a page is destroyed while a render request is active~~
   __DONE__
 - ~~GotoPage should not center on page~~ __DONE__
 - ~~zoom/hand tools~~ __DONE__
 - ~~ctrl+mousewheel should zoom~~ __DONE__
 - ~~maquee zoom (requested by Philip Taylor)~~ __DONE__
 - ~~Flesh out link support~~ __DONE__
 - ~~Add some loading indicators so that users know something is happening in the
   background when pages are blank or blurry~~ __DONE__
 - ~~scrolling beyond edge at single page mode (mouse wheel/up/down)~~ __DONE__
 - ~~disallow keyboard events in document view while magnifier is shown (otherwise
   one could scroll the canvas without properly adjusting the magnifier)~~ __DONE__
 - ~~SyncTeX with signals and slots~~ __DONE__ (actually invoking SyncTeX must
   be implemented separate from the viewer)
 - ~~fit to width/page/~~ __DONE__
 - ~~PgDn should scroll one viewport height~~ __DONE__
 - ~~draw frame around magnifying glass (requested by Reinhard Kotucha)~~ __DONE__
 - ~~Control memory usage through caching and zoom throttling~~ __DONE__
 - ~~Don't show mouse cursor while using the magnifier~~ __DONE__

### Wishlist
 - Provide a grayscale mode
 - Speed up magnifier; in Tw, it seems much more responsive. This might be due
   to using centerOn in PDFDocumentMagnifierView::setPosition but needs closer
   investigation
 - Hitting PgUp at top of document or PgDn at bottom moves to the right
 - annotations (popup window)
 - make the magnifying glass a top-level window (so it can extend outside the
   main view's window boundaries) (requested by Reinhard Kotucha)
 - rulers (in main window, and attached to magnifying glass)
 - or measuring tool
 - Split view???
 - Provide additional info, e.g. through QDockWidget. Annotations, thumbnails (?).
 - Printing.
 - Derive zoom icons from Tango instead of from Qt
 - Presentation mode
   - should display all pages in fit-to-window mode
   - must render all pages completely before switching to them
   - full screen
   - transition support?
 - save page as image
 - selecting (and copying/saving) text & images (subject to permissions)
