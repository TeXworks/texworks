/**
 * Copyright (C) 2013-2023  Charlie Sharpsteen, Stefan Löffler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */
#include <iostream>

#include "PDFViewer.h"

#if defined(STATIC_QT5) && defined(Q_OS_WIN32)
  #include <QtPlugin>
  Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

int main(int argc, char **argv) {
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  QCoreApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);
#if QT_VERSION >= 0x050600
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
#endif

  QApplication app(argc, argv);
  QIcon::setThemeName(QStringLiteral("tango-qtpdf"));
  app.setWindowIcon(QIcon::fromTheme(QStringLiteral("QtPDF")));
  PDFViewer mainWin(QString::fromUtf8("pgfmanual.pdf"));

  mainWin.show();
  return QApplication::exec();
  //return 0;
}
