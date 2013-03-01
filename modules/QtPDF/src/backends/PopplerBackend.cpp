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
  _poppler_doc(Poppler::Document::load(fileName))
{
  _numPages = _poppler_doc->numPages();

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

QImage PopplerPage::renderToImage(double xres, double yres)
{
  return QImage();
}


// vim: set sw=2 ts=2 et

