#include <iostream>

#include "PDFViewer.h"


int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(QString::fromUtf8(":/icons/logo.png")));
  PDFViewer mainWin(QString::fromUtf8("pgfmanual.pdf"));

  mainWin.show();
  return app.exec();
  //return 0;
}
