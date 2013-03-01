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
#ifndef PDFDOCUMENTWIDGET_H
#define PDFDOCUMENTWIDGET_H

#include "PDFDocumentView.h"

namespace QtPDF {

class PDFDocumentWidget : public PDFDocumentView
{
	Q_OBJECT
public:
  PDFDocumentWidget(QWidget * parent = NULL);
  virtual ~PDFDocumentWidget();

  bool load(const QString & filename);

  // *TODO*: Possibly add some way to describe/choose/change the PDF backend used

protected:
  QSharedPointer<QtPDF::PDFDocumentScene> _scene;
};

} // namespace QtPDF

#endif // PDFDOCUMENTWIDGET_H
