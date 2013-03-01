/**
 * Copyright (C) 2011-2012  Charlie Sharpsteen, Stefan Löffler
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

namespace QtPDF {

namespace Backend {

namespace MuPDF {

class Document;
class Page;

class Document: public Backend::Document
{
  typedef Backend::Document Super;
  friend class Page;

  void recursiveConvertToC(QList<PDFToCItem> & items, pdf_outline * node) const;

protected:
  // The pdf_xref is the main MuPDF object that represents a Document. Calls
  // that use it may have to be protected by a mutex.
  pdf_xref *_mupdf_data;
  fz_glyph_cache *_glyph_cache;

  void loadMetaData();

public:
  Document(QString fileName);
  ~Document();

  bool isValid() const { return (_mupdf_data != NULL); }
  bool isLocked() const { return (isValid() && _permissionLevel == PermissionLevel_Locked); }

  bool unlock(const QString password);
  void reload();

  QSharedPointer<Backend::Page> page(int at);
  QSharedPointer<Backend::Page> page(int at) const;
  PDFDestination resolveDestination(const PDFDestination & namedDestination) const;

  PDFToC toc() const;
  QList<PDFFontInfo> fonts() const;

private:
  enum PermissionLevel { PermissionLevel_Locked, PermissionLevel_User, PermissionLevel_Owner };
  QString _password;
  // NOTE: the MuPDF function pdf_needs_password actually tries to authenticate
  // with an empty password under certain circumstances. This can effectively
  // relock _mupdf_data, so we have to cache to `locked` state.
  PermissionLevel _permissionLevel;
};


class Page: public Backend::Page
{
  typedef Backend::Page Super;

  // The `fz_display_list` is the main MuPDF object that represents the parsed
  // contents of a Page.
  fz_display_list *_mupdf_page;

  // Keep as a Fitz object rather than QRect as it is used in rendering ops.
  fz_rect _bbox;
  QSizeF _size;
  qreal _rotate;

  QList< QSharedPointer<Annotation::AbstractAnnotation> > _annotations;
  QList< QSharedPointer<Annotation::Link> > _links;
  bool _annotationsLoaded;
  bool _linksLoaded;
  
  void loadTransitionData();

public:
  Page(Document *parent, int at);
  ~Page();

  QSizeF pageSizeF() const;

  QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false);

  QList< QSharedPointer<Annotation::Link> > loadLinks();
  QList< QSharedPointer<Annotation::AbstractAnnotation> > loadAnnotations();

  QList<SearchResult> search(QString searchText);
  virtual QList<Backend::Page::Box> boxes();
  virtual QString selectedText(const QList<QPolygonF> & selection);
};

} // namespace MuPDF

} // namespace Backend

} // namespace QtPDF

#endif // End header guard
// vim: set sw=2 ts=2 et

