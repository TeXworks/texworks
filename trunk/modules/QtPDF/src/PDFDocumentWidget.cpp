/**
 * Copyright (C) 2012  Stefan Löffler
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
#include "PDFDocumentWidget.h"

namespace QtPDF {

PDFDocumentWidget::PDFDocumentWidget(QWidget * parent /* = NULL */)
: PDFDocumentView(parent)
{
#ifdef USE_MUPDF
  _backends.append(new MuPDFBackend());
#endif
#ifdef USE_POPPLERQT4
  _backends.append(new PopplerQt4Backend());
#endif
}

PDFDocumentWidget::~PDFDocumentWidget()
{
  foreach(BackendInterface * bi, _backends) {
    if (!bi)
      continue;
    bi->deleteLater();
  }
}

// Loads the file specified by filename. If this succeeds, the new file is
// displayed and true is returned. Otherwise, the view is not altered and false
// is returned
bool PDFDocumentWidget::load(const QString &filename)
{
  QSharedPointer<QtPDF::Backend::Document> a_pdf_doc;
  foreach(BackendInterface * bi, _backends) {
    if (bi && bi->canHandleFile(filename))
      a_pdf_doc = bi->newDocument(filename);
    if (a_pdf_doc)
      break;
  }

  if (!a_pdf_doc || !a_pdf_doc->isValid())
    return false;

  // Note: Don't pass `this` (or any other QObject*) as parent to the new
  // PDFDocumentScene as that would cause docScene to be destroyed with its
  // parent, thereby bypassing the QSharedPointer mechanism. docScene will be
  // freed automagically when the last QSharedPointer pointing to it will be
  // destroyed.
  _scene = QSharedPointer<QtPDF::PDFDocumentScene>(new QtPDF::PDFDocumentScene(a_pdf_doc));
  setScene(_scene);
  return true;
}

QStringList PDFDocumentWidget::backends() const
{
  QStringList retVal;
  foreach (BackendInterface * bi, _backends) {
    if (bi)
      continue;
    retVal << bi->name();
  }
  return retVal;
}

QString PDFDocumentWidget::defaultBackend() const
{
  if (_backends.size() == 0)
    return QString();
  return _backends[0]->name();
}

void PDFDocumentWidget::setDefaultBackend(const QString & backend)
{
  int i;
  for (i = 0; i < _backends.size(); ++i) {
    if (_backends[i]->name() == backend)
      break;
  }
  if (i < _backends.size()) {
    _backends.move(i, 0);
  }
}


} // namespace QtPDF
