/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3, or (at your option) any later
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
#ifndef MuPDFBackend_H
#define MuPDFBackend_H

extern "C"
{
#include <fitz.h>
#include <mupdf.h>
}

class MuPDFDocument;
class MuPDFPage;

class MuPDFDocument: public Document
{
  typedef Document Super;
  friend class MuPDFPage;

  void recursiveConvertToC(QList<PDFToCItem> & items, pdf_outline * node) const;

protected:
  // The pdf_xref is the main MuPDF object that represents a Document. Calls
  // that use it may have to be protected by a mutex.
  pdf_xref *_mupdf_data;
  fz_glyph_cache *_glyph_cache;

  void loadMetaData();

public:
  MuPDFDocument(QString fileName);
  ~MuPDFDocument();

  QSharedPointer<Page> page(int at);
  PDFDestination resolveDestination(const PDFDestination & namedDestination) const;

  PDFToC toc() const;
  QList<PDFFontInfo> fonts() const;
};


class MuPDFPage: public Page
{
  typedef Page Super;

  // The `fz_display_list` is the main MuPDF object that represents the parsed
  // contents of a Page.
  fz_display_list *_mupdf_page;

  // Keep as a Fitz object rather than QRect as it is used in rendering ops.
  fz_rect _bbox;
  QSizeF _size;
  qreal _rotate;

  QList< QSharedPointer<PDFLinkAnnotation> > _links;
  bool _linksLoaded;

public:
  MuPDFPage(MuPDFDocument *parent, int at);
  ~MuPDFPage();

  QSizeF pageSizeF();

  QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false);

  QList< QSharedPointer<PDFLinkAnnotation> > loadLinks();

  QList<SearchResult> search(QString searchText);
};


#endif // End header guard
// vim: set sw=2 ts=2 et

