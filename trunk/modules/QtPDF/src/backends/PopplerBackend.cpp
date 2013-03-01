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

// NOTE: `PopplerBackend.h` is included via `PDFBackend.h`
#include <PDFBackend.h>

// Document Class
// ==============
PopplerDocument::PopplerDocument(QString fileName):
  Super(fileName),
  _poppler_doc(Poppler::Document::load(fileName)),
  _doc_lock(new QMutex())
{
  _numPages = _poppler_doc->numPages();

  // **TODO:**
  //
  // _Make these configurable._
  _poppler_doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  _poppler_doc->setRenderHint(Poppler::Document::Antialiasing);
  _poppler_doc->setRenderHint(Poppler::Document::TextAntialiasing);
}

PopplerDocument::~PopplerDocument()
{
}

Page *PopplerDocument::page(int at){ return new PopplerPage(this, at); }


// Page Class
// ==========
PopplerPage::PopplerPage(PopplerDocument *parent, int at):
  Super(parent, at),
  _parent(parent),
  _poppler_page(_parent->_poppler_doc->page(at))
{
}

PopplerPage::~PopplerPage()
{
}

QSizeF PopplerPage::pageSizeF() { return _poppler_page->pageSizeF(); }

QImage PopplerPage::renderToImage(double xres, double yres, int x, int y, int width, int height)
{
  QImage renderedPage;

  // Rendering pages is not thread safe.
  QMutexLocker docLock(_parent->_doc_lock);
    renderedPage = _poppler_page->renderToImage(xres, yres, x, y, width, height);
  docLock.unlock();

  return renderedPage;
}


// vim: set sw=2 ts=2 et

