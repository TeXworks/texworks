#include <iostream>

#include <QtCore/QtCore>

#include "PDFView.h"


int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QMainWindow mainWin;

  Poppler::Document *doc = Poppler::Document::load(QString("pgfmanual.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  PDFDocumentView *docView = new PDFDocumentView(doc, &mainWin);
  mainWin.setCentralWidget(docView);

  QToolBar *toolBar = new QToolBar(&mainWin);
  toolBar->addAction("Zoom In", docView, SLOT(zoomIn()));
  toolBar->addAction("Zoom Out", docView, SLOT(zoomOut()));
  mainWin.addToolBar(toolBar);

  PageCounter *counter = new PageCounter(&mainWin);
  counter->setLastPage(docView->lastPage());
  QObject::connect(docView, SIGNAL(changedPage(int)), counter, SLOT(setCurrentPage(int)));
  mainWin.statusBar()->addPermanentWidget(counter);

  ZoomTracker *zoomWdgt = new ZoomTracker(&mainWin);
  QObject::connect(docView, SIGNAL(changedZoom(qreal)), zoomWdgt, SLOT(setZoom(qreal)));
  mainWin.statusBar()->addWidget(zoomWdgt);

  mainWin.show();
  return app.exec();
  //return 0;
}
