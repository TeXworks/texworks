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


// Backend Rendering
// =================

// FIXME: Forward-declaring these classes is a hack that allows the threaded
// rendering code to live in PDFBackend. Clean up and fix.
class PDFPageGraphicsItem;
class PDFLinkGraphicsItem;

class PageProcessingRequest : public QObject
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRequest(PDFPageGraphicsItem * page) : page(page) { }
  // Should perform whatever processing it is designed to do
  // Returns true if finished successfully, false otherwise
  virtual bool execute() = 0;

public:
  enum Type { PageRendering, LoadLinks };

  virtual ~PageProcessingRequest() { }
  virtual Type type() = 0;

  PDFPageGraphicsItem * page;
};


class PageProcessingRenderPageRequest : public PageProcessingRequest
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRenderPageRequest(PDFPageGraphicsItem * page, qreal scaleFactor);
  virtual bool execute();

public:
  virtual Type type() { return PageRendering; }

  qreal scaleFactor;

signals:
  void pageImageReady(qreal, QImage);
};


class PageProcessingLoadLinksRequest : public PageProcessingRequest
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingLoadLinksRequest(PDFPageGraphicsItem * page) : PageProcessingRequest(page) { }
  virtual bool execute();

public:
  virtual Type type() { return LoadLinks; }

signals:
  void linksReady(QList<PDFLinkGraphicsItem *>);
};


// Class to perform (possibly) lengthy operations on pages in the background
// Modelled after the "Blocking Fortune Client Example" in the Qt docs
// (http://doc.qt.nokia.com/stable/network-blockingfortuneclient.html)
class PDFPageProcessingThread : public QThread
{
  Q_OBJECT
public:
  PDFPageProcessingThread();
  virtual ~PDFPageProcessingThread();

  PageProcessingRenderPageRequest* requestRenderPage(PDFPageGraphicsItem * page, const qreal scaleFactor) const;
  PageProcessingLoadLinksRequest* requestLoadLinks(PDFPageGraphicsItem * page) const;

  // add a processing request to the work stack
  // Note: request must have been created on the heap and must be in the scope
  // of this thread; use requestRenderPage() and requestLoadLinks() for that
  // Note: this must be separate from the request...() methods to allow the
  // calling thread some late initialization (e.g., connecting to signals)
  void addPageProcessingRequest(PageProcessingRequest * request);

protected:
  virtual void run();

private:
  QStack<PageProcessingRequest*> _workStack;
  QMutex _mutex;
  QWaitCondition _waitCondition;
  bool _quit;
#ifdef DEBUG
  QTime _renderTimer;
#endif
};


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
  const int _n;

public:
  Page(Document *parent, int at);
  ~Page();

  int pageNum();
  virtual QSizeF pageSizeF()=0;

  virtual QImage renderToImage(double xres, double yres, int x=-1, int y=-1, int width=-1, int height=-1)=0;

};


// Backend Implementations
// =======================
// These provide library-specific concrete impelemntations of the abstract base
// classes defined here.
#include <backends/PopplerBackend.h>


#endif // End header guard
// vim: set sw=2 ts=2 et

