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


// Backend Rendering
// =================
// The `PDFPageProcessingThread` is a thread that processes background jobs.
// Each job is represented by a subclass of `PageProcessingRequest` and
// contains an `execute` method that performs the actual work.
PDFPageProcessingThread::PDFPageProcessingThread() :
_quit(false)
{
}

PDFPageProcessingThread::~PDFPageProcessingThread()
{
  _mutex.lock();
  _quit = true;
  _waitCondition.wakeAll();
  _mutex.unlock();
  wait();
}

void PDFPageProcessingThread::requestRenderPage(Page *page, QObject *listener, double xres, double yres, QRect render_box)
{
  addPageProcessingRequest(new PageProcessingRenderPageRequest(page, listener, xres, yres, render_box));
}

void PDFPageProcessingThread::requestLoadLinks(Page *page, QObject *listener)
{
  addPageProcessingRequest(new PageProcessingLoadLinksRequest(page, listener));
}

void PDFPageProcessingThread::addPageProcessingRequest(PageProcessingRequest * request)
{
  int i;

  if (!request)
    return;

  QMutexLocker locker(&(this->_mutex));
  // remove any instances of the given request type before adding the new one to
  // avoid processing it several times
  // **TODO:** Could it be that we require several concurrent versions of the
  //           same page?
  for (i = _workStack.size() - 1; i >= 0; --i) {
    if (_workStack[i]->page == request->page && _workStack[i]->type() == request->type()) {
      // Using deleteLater() doesn't work because we have no event queue in this
      // thread. However, since the object is still on the stack, it is still
      // sleeping and directly deleting it should therefore be safe.
      delete _workStack[i];
      _workStack.remove(i);
    }
  }

  _workStack.push(request);
  locker.unlock();
#ifdef DEBUG
  QString jobDesc;
  switch (request->type()) {
    case PageProcessingRequest::LoadLinks:
      jobDesc = QString::fromUtf8("loading links request");
      break;
    case PageProcessingRequest::PageRendering:
      jobDesc = QString::fromUtf8("rendering page request");
      break;
  }
  qDebug() << "new" << jobDesc << "for page" << request->page->pageNum();
#endif

  if (!isRunning())
    start();
  else
    _waitCondition.wakeOne();
}

void PDFPageProcessingThread::run()
{
  PageProcessingRequest * workItem;

  _mutex.lock();
  while (!_quit) {
    // mutex must be locked at start of loop
    if (_workStack.size() > 0) {
      workItem = _workStack.pop();
      _mutex.unlock();

#ifdef DEBUG
      qDebug() << "processing work item; remaining items:" << _workStack.size();
      _renderTimer.start();
#endif
      workItem->execute();
#ifdef DEBUG
      QString jobDesc;
      switch (workItem->type()) {
        case PageProcessingRequest::LoadLinks:
          jobDesc = QString::fromUtf8("loading links");
          break;
        case PageProcessingRequest::PageRendering:
          jobDesc = QString::fromUtf8("rendering page");
          break;
      }
      qDebug() << "finished " << jobDesc << "for page" << workItem->page->pageNum() << ". Time elapsed: " << _renderTimer.elapsed() << " ms.";
#endif

      // Delete the work item as it has fulfilled its purpose
      // Note that we can't delete it here or we might risk that some emitted
      // signals are invalidated; to ensure they reach their destination, we
      // need to call deleteLater(), which requires and event queue; thus, we
      // first move it to the main processing thread
      workItem->moveToThread(QApplication::instance()->thread());
      workItem->deleteLater();

      _mutex.lock();
    }
    else {
#ifdef DEBUG
      qDebug() << "going to sleep";
#endif
      _waitCondition.wait(&_mutex);
#ifdef DEBUG
      qDebug() << "waking up";
#endif
    }
  }
}


// Asynchronous Page Operations
// ----------------------------
//
// The `execute` functions here are called by the processing theread to perform
// background jobs such as page rendering or link loading. This alows the GUI
// thread to stay unblocked and responsive. The results of background jobs are
// posted as events to a `listener` which can be any subclass of `QObject`. The
// `listener` will need a custom `event` function that is capable of picking up
// on these events.


// ### Custom Event Types
// These are the events posted by `execute` functions.
const QEvent::Type PDFPageRenderedEvent::PageRenderedEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );
const QEvent::Type PDFLinksLoadedEvent::LinksLoadedEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );

bool PageProcessingRenderPageRequest::execute()
{
  // FIXME:
  // Aborting renders doesn't really work right now---the backend knows nothing
  // about the PDF scenes.
  //
  // Idea: Perhaps allow page render requests to provide a pointer to a function
  // that returns a `bool` value indicating if the request is still valid? Then
  // the `PDFPageGraphicsItem` could have a function that indicates if the item
  // is anywhere near a viewport.
  QImage rendered_page = page->renderToImage(xres, yres, render_box);
  QCoreApplication::postEvent(listener, new PDFPageRenderedEvent(xres, yres, render_box, rendered_page));

  return true;
}

bool PageProcessingLoadLinksRequest::execute()
{
  QCoreApplication::postEvent(listener, new PDFLinksLoadedEvent(page->loadLinks()));
  return true;
}


// PDF ABCs
// ========

// Document Class
// --------------
Document::Document(QString fileName):
  _numPages(-1)
{
}

Document::~Document()
{
}

int Document::numPages() { return _numPages; }
PDFPageProcessingThread &Document::processingThread() { return _processingThread; }


// Page Class
// ----------
Page::Page(Document *parent, int at):
  _parent(parent),
  _n(at)
{
}

Page::~Page()
{
}

int Page::pageNum() { return _n; }


// vim: set sw=2 ts=2 et

