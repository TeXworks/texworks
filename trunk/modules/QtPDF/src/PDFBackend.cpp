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

#include <PDFBackend.h>

// Document Class
// ==============
Document::Document(QString fileName):
  _numPages(-1)
{
}

Document::~Document()
{
}

int Document::numPages() { return _numPages; }


// Page Class
// ==========
Page::Page(Document *parent, int at):
  _n(at)
{
}

Page::~Page()
{
}

int Page::pageNum() { return _n; }

// vim: set sw=2 ts=2 et

