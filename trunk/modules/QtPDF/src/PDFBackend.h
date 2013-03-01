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

// FIXME: Thin the header inclusion down.
#include <QtCore>
#include <QImage>
#include <QApplication>


// Backend Rendering
// =================

// FIXME: Forward-declaring these classes is a hack that allows the threaded
// rendering code to live in PDFBackend. Clean up and fix.
class Page;
class Document;
// FIXME: Loading the Poppler Qt4 headers here to gain access to Link class in
// order to test the rendering thread. We need a link class of our own.
#include <poppler/qt4/poppler-qt4.h>


class PageProcessingRequest : public QObject
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRequest(Page *page, QObject *listener) : page(page), listener(listener) { }
  // Should perform whatever processing it is designed to do
  // Returns true if finished successfully, false otherwise
  virtual bool execute() = 0;

public:
  enum Type { PageRendering, LoadLinks };

  virtual ~PageProcessingRequest() { }
  virtual Type type() = 0;

  Page *page;
  QObject *listener;
};


class PageProcessingRenderPageRequest : public PageProcessingRequest
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRenderPageRequest(Page *page, QObject *listener, qreal scaleFactor);
  bool execute();

public:
  Type type() { return PageRendering; }

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
  PageProcessingLoadLinksRequest(Page *page, QObject *listener) : PageProcessingRequest(page, listener) { }
  bool execute();

public:
  Type type() { return LoadLinks; }

};


class PDFLinksLoadedEvent : public QEvent
{
  typedef QEvent Super;

public:
  PDFLinksLoadedEvent(const QList<Poppler::Link *> links): Super(LinksLoadedEvent), links(links) {}
  static QEvent::Type LinksLoadedEvent;
  const QList<Poppler::Link *> links;
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

  void requestRenderPage(Page *page, QObject *listener, qreal scaleFactor);
  void requestLoadLinks(Page *page, QObject *listener);

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

class Document
{
  friend class Page;

protected:
  int _numPages;
  PDFPageProcessingThread _processingThread;

public:
  Document(QString fileName);
  ~Document();

  int numPages();
  PDFPageProcessingThread& processingThread();

  virtual Page *page(int at)=0;

};

class Page
{

protected:
  Document *_parent;
  const int _n;

public:
  Page(Document *parent, int at);
  ~Page();

  int pageNum();
  virtual QSizeF pageSizeF()=0;

  virtual QImage renderToImage(double xres, double yres, int x=-1, int y=-1, int width=-1, int height=-1)=0;

  virtual QList<Poppler::Link *> loadLinks()=0;
  virtual void asyncLoadLinks(QObject *listener)=0;

};


// Backend Implementations
// =======================
// These provide library-specific concrete impelemntations of the abstract base
// classes defined here.
#include <backends/PopplerBackend.h>


#endif // End header guard
// vim: set sw=2 ts=2 et

