/**
 * Copyright (C) 2011  Charlie Sharpsteen
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
#ifndef PDFBackend_H
#define PDFBackend_H

// TODO: Thin the header inclusion down.
#include <QtCore>
#include <QImage>

// PDF ABCs
// ========
// This header file defines a set of Abstract Base Classes (ABCs) for PDF
// documents. Having a set of abstract classes allows tools like GUI viewers to
// be written that are agnostic to the library that provides the actual PDF
// implementation: Poppler, MuPDF, etc.
class Document;
class Page;

class Document
{
  friend class Page;

protected:
  int _numPages;

public:
  Document(QString fileName);
  ~Document();

  int numPages();
  virtual Page *page(int at)=0;

};


class Page
{
protected:
  QSizeF _size;
  qreal _rotate;
  const int _n;

public:
  Page(Document *parent, int at);
  ~Page();

  int pageNum();
  qreal rotate();
  QSizeF pageSizeF();

  virtual QImage renderToImage(double xres, double yres)=0;

};


// Backend Implementations
// =======================
// These provide library-specific concrete impelemntations of the abstract base
// classes defined here.
#include <backends/PopplerBackend.h>


#endif // End header guard
// vim: set sw=2 ts=2 et

