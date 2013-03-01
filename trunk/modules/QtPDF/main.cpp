#include <iostream>

#include "PDFViewer.h"


int main(int argc, char **argv) {
  QApplication app(argc, argv);
  PDFViewer mainWin(QString::fromUtf8("pgfmanual.pdf"));

  mainWin.show();
  return app.exec();
  //return 0;
}
