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
 - ~~GotoPage should not center on page~~ __DONE__
 - ~~zoom/hand tools~~ __DONE__
 - ~~ctrl+mousewheel should zoom~~ __DONE__
 - ~~maquee zoom (requested by Philip Taylor)~~ __DONE__
 - context menu
 - ~~Flesh out link support~~ __DONE__
 - ~~Add some loading indicators so that users know something is happening in the
   background when pages are blank or blurry~~ __DONE__
 - scrolling beyond edge at single page mode (mouse wheel/up/down)
 - ~~disallow keyboard events in document view while magnifier is shown (otherwise
   one could scroll the canvas without properly adjusting the magnifier)~~ __DONE__
 - ~~SyncTeX with signals and slots~~ __DONE__ (actually invoking SyncTeX must
   be implemented separate from the viewer)
 - ~~fit to width/page/~~ __DONE__
 - Highlighting! (to show syncing destination, search results, etc.)
 - ~~PgDn should scroll one viewport height~~ __DONE__
 - ~~draw frame around magnifying glass (requested by Reinhard Kotucha)~~ __DONE__
 - Comment on how `pageScale` works and is used
 - ~~Control memory usage through caching and zoom throttling~~ __DONE__
 - possibility to abort render requests when page moves out of view (is there a
   way to avoid going through all pages at each scroll event?)
 - Program segfaults if a page is destroyed while a render request is active
 - Possibly simplify page processing request generation. Rational: Right now,
   requestLoadLinks, requestRenderPage, and addPageProcessingRequest are all
   called from the main thread, if I understood threading correctly.
   Consequently, the page processing request object should live in the main
   thread as well (but is accessed only from the worked thread). If that is
   correct, all the moving of objects is superfluous as well.
 - Turn PDFViewer into a more general test case (loading of arbitrary files,
   etc.)
 - Implement text search for PDF files.

### Wishlist
 - annotations (popup window)
 - make the magnifying glass a top-level window (so it can extend outside the
   main view's window boundaries) (requested by Reinhard Kotucha)
 - rulers (in main window, and attached to magnifying glass)
 - Split view???
 - Improve handling of several concurrent versions (magnifications) of the same
   page (currently: "normal" and "magnified" versions); simplify code, remove
   redundancy, etc.
 - Printing.
