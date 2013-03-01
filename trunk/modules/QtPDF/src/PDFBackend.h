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

class Page;
class Document;

// FIXME:
// Loading the Poppler Qt4 headers here to gain access to Link class in
// order to test the rendering thread. We need a link class of our own.
#include <poppler/qt4/poppler-qt4.h>

class PDFPageTile
{

public:
  // TODO:
  // We may want an application-wide cache instead of a document-specific cache
  // to keep memory usage down. This may require an additional piece of
  // information---the document that the page belongs to.
  PDFPageTile(double xres, double yres, QRect render_box, int page_num):
    xres(xres), yres(yres),
    render_box(render_box),
    page_num(page_num)
  {}

  double xres, yres;
  QRect render_box;
  int page_num;

  bool operator==(const PDFPageTile &other) const
  {
    return (xres == other.xres && yres == other.yres && render_box == other.render_box && page_num == other.page_num);
  }

};
// Need a hash function in order to allow `PDFPageTile` to be used as a key
// object for a `QCache`.
uint qHash(const PDFPageTile &tile);

// TODO: May need a full subclass to do things like return the nearest image if
// a specific resolution is not available.
typedef QCache<PDFPageTile, QImage> PDFPageCache;


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
  PageProcessingRenderPageRequest(Page *page, QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false) :
    PageProcessingRequest(page, listener),
    xres(xres), yres(yres),
    render_box(render_box),
    cache(cache)
  {}

  bool execute();

public:
  Type type() { return PageRendering; }

  double xres, yres;
  QRect render_box;
  bool cache;

};


class PDFPageRenderedEvent : public QEvent
{

public:
  PDFPageRenderedEvent(double xres, double yres, QRect render_rect, QImage rendered_page):
    QEvent(PageRenderedEvent),
    xres(xres), yres(yres),
    render_rect(render_rect),
    rendered_page(rendered_page)
  {}

  static const QEvent::Type PageRenderedEvent;

  const double xres, yres;
  const QRect render_rect;
  const QImage rendered_page;

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

public:
  PDFLinksLoadedEvent(const QList<Poppler::Link *> links):
    QEvent(LinksLoadedEvent),
    links(links)
  {}

  static const QEvent::Type LinksLoadedEvent;

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

  void requestRenderPage(Page *page, QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false);
  void requestLoadLinks(Page *page, QObject *listener);

  // add a processing request to the work stack
  // Note: request must have been created on the heap and must be in the scope
  // of this thread; use requestRenderPage() and requestLoadLinks() for that
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
  PDFPageCache _pageCache;

public:
  Document(QString fileName);
  ~Document();

  int numPages();
  PDFPageProcessingThread& processingThread();
  PDFPageCache& pageCache();

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

  virtual QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false)=0;
  virtual void asyncRenderToImage(QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false);

  virtual QList<Poppler::Link *> loadLinks()=0;
  virtual void asyncLoadLinks(QObject *listener);

  QImage *getCachedImage(double xres, double yres, QRect render_box = QRect());

};


// Backend Implementations
// =======================
// These provide library-specific concrete impelemntations of the abstract base
// classes defined here.
#include <backends/PopplerBackend.h>


#endif // End header guard
// vim: set sw=2 ts=2 et

