/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
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

// NOTE:
//
// ** THIS HEADER IS NOT MENT TO BE INCLUDED DIRECTLY **
//
// Instead, include `PDFBackend.h` which defines classes that this header
// relies on.
#ifndef PopplerBackend_H
#define PopplerBackend_H

#include <poppler/qt4/poppler-qt4.h>

class PopplerDocument;
class PopplerPage;

class PopplerDocument: public Document
{
  typedef Document Super;
  friend class PopplerPage;

  QSharedPointer<Poppler::Document> _poppler_doc;

protected:
  // Poppler is not threadsafe, so some operations need to be serialized with a
  // mutex.
  QMutex *_doc_lock;

public:
  PopplerDocument(QString fileName);
  ~PopplerDocument();

  Page *page(int at);

};


class PopplerPage: public Page
{
  typedef Page Super;
  QSharedPointer<Poppler::Page> _poppler_page;
  QList< QSharedPointer<PDFLinkAnnotation> > _links;
  bool _linksLoaded;

public:
  PopplerPage(PopplerDocument *parent, int at);
  ~PopplerPage();

  QSizeF pageSizeF();

  QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false);

  QList< QSharedPointer<PDFLinkAnnotation> > loadLinks();
};


#endif // End header guard
// vim: set sw=2 ts=2 et

