#include <iostream>

#include <QtCore/QtCore>

#include "PDFView.h"


int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QMainWindow mainWin;

  Poppler::Document *doc = Poppler::Document::load(QString("pgfmanual.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  PDFDocumentView *docView = new PDFDocumentView(doc, &mainWin);

  PageCounter *counter = new PageCounter(&mainWin);
  counter->setLastPage(docView->lastPage());

  mainWin.setCentralWidget(docView);
  QObject::connect(docView, SIGNAL(changedPage(int)), counter, SLOT(setCurrentPage(int)));
  mainWin.statusBar()->addPermanentWidget(counter);
  mainWin.show();
  return app.exec();
  //return 0;
}
