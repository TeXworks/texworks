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


// Custom Event Types
// ==================

QEvent::Type PDFLinksLoadedEvent::LinksLoadedEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );


// Backend Rendering
// =================

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

void PDFPageProcessingThread::requestRenderPage(Page *page, QObject *listener, qreal scaleFactor)
{
  addPageProcessingRequest(new PageProcessingRenderPageRequest(page, listener, scaleFactor));
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
      qDebug() << "finished " << jobDesc << "for page" << workItem->page->pageNum();
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


PageProcessingRenderPageRequest::PageProcessingRenderPageRequest(Page *page, QObject *listener, qreal scaleFactor) :
  PageProcessingRequest(page, listener),
  scaleFactor(scaleFactor)
{
}

bool PageProcessingRenderPageRequest::execute()
{
  // Set up to report failure. All objects that recieve the `pageImageReady`
  // signal emitted by this class should check to see that the QImage passed is
  // not empty using `QImage.isNull`. if it is empty, that means the render
  // failed or the request decided to abort.
  bool doRender = true;
  QImage pageImage = QImage();

  // FIXME:
  // Aborting renders doesn't really work right now---the backend knows nothing
  // about the PDF scenes.

  //PDFDocumentScene *pageScene = qobject_cast<PDFDocumentScene *>(page->scene());
  //QPointF pageCenter, viewCenter;
  //// The render tolerance is used to control the maximum distance from the
  //// center of the viewport at which pages will render.
  //qreal pageHeight, pageDistance, RENDER_TOLERANCE = 5.0;

  //if (!page || !qobject_cast<PDFDocumentScene *>(page->scene()) || !page->_page)
    //goto renderReport; // Reports failure.

  //// Check to see that the page is visible by at least one of the views
  //// observing the scene. If not, skip rendering.
  ////
  //// **TODO:**
  ////
  //// Should this logic be executed at a higher level? Perhaps by the rendering
  //// thread before it decides to process a request? Two problems with this:
  ////
  ////   - Most objects expect to recieve some sort of signal after submitting
  ////     a job and these signals are emitted in the `execute` function.
  ////
  ////   - The `PDFPageGraphicsItem` sets `_linksLoaded` to `true` after
  ////     the load request, not when the results are recieved.
  ////
  //// This wont work for single page layout modes because the pages are all
  //// stacked on top of each other, so it always returns true. Need a better
  //// check for this case.
  //pageCenter = page->sceneBoundingRect().center();
  //pageHeight = page->sceneBoundingRect().height();

  //foreach ( QGraphicsView *view, pageScene->views() )
  //{
    //if ( view->isHidden() )
      //continue; // Go to the next iteration if a view is not visible to the user.

    //viewCenter = view->mapToScene(view->rect()).boundingRect().center();
    //pageDistance = QLineF(pageCenter, viewCenter).length();
    //// If the distance between the center of the page and the center of the
    //// viewport is less than a certain multiple of the page height, we will
    //// render the page.
    //if ( pageDistance < pageHeight * RENDER_TOLERANCE ) {
      //doRender = true;
      //break;
    //}
  //}

  if ( doRender )
    // FIXME: Just using the scale factor is not right. Should probably pass
    // DPI into this function instead. This is a hack to allow compilation
    // because _dpiX and _dpiY are currently unavailable. Fix this when async
    // page rendering is re-implemented.
    //
    // pageImage = page->renderToImage(page->_dpiX * scaleFactor, page->_dpiY * scaleFactor);
    pageImage = page->renderToImage(scaleFactor, scaleFactor);

renderReport:
  // FIXME: Replace the signal with an event that can be recieved by any
  // `QObject`
  emit pageImageReady(scaleFactor, pageImage);
  return doRender;
}


// Asynchronous Link Generation
// ----------------------------

// This function generates `PDFLinkGraphicsItem` objects. It is intended to be
// called asynchronously and so does not set parentage for the objects it
// generates --- this task is left to the `addLinks` method so that all the
// links are added and rendered in a synchronous operation.
bool PageProcessingLoadLinksRequest::execute()
{
  QCoreApplication::postEvent(listener, new PDFLinksLoadedEvent(page->loadLinks()));
  return true;
}


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
PDFPageProcessingThread &Document::processingThread() { return _processingThread; }


// Page Class
// ==========
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

