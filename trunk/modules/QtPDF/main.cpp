#include <iostream>

#include "PDFViewer.h"

#if defined(STATIC_QT5) && defined(Q_OS_WIN32)
  #include <QtPlugin>
  Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

int main(int argc, char **argv) {
  QApplication app(argc, argv);
  app.setWindowIcon(QIcon(QString::fromUtf8(":/icons/logo.png")));
  PDFViewer mainWin(QString::fromUtf8("pgfmanual.pdf"));

  mainWin.show();
  return app.exec();
  //return 0;
}
