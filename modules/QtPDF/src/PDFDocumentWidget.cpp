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
}

PDFDocumentWidget::~PDFDocumentWidget()
{
}

// Loads the file specified by filename. If this succeeds, the new file is
// displayed and true is returned. Otherwise, the view is not altered and false
// is returned
bool PDFDocumentWidget::load(const QString &filename)
{
  // *TODO*: If more than one backend is available, maybe let users set their
  //          preferred one
#ifdef USE_MUPDF
  QSharedPointer<QtPDF::Backend::Document> a_pdf_doc(new QtPDF::Backend::MuPDF::Document(filename));
#elif USE_POPPLER
  QSharedPointer<QtPDF::Backend::Document> a_pdf_doc(new QtPDF::Backend::Poppler::Document(filename));
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

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

} // namespace QtPDF
