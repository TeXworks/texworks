/**
 * Copyright 2011 Charlie Sharpsteen
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
#include "PDFView.h"
#include <iostream>


// PDFPageGraphicsItem
// ===================

// This class descends from `QGraphicsPixmapItem` and is responsible for
// rendering `Poppler::Page` objects.
PDFPageGraphicsItem::PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent) : super(parent),
  page(a_page),
  dirty(true),
  dpiX(QApplication::desktop()->physicalDpiX()),
  dpiY(QApplication::desktop()->physicalDpiY())
{
  // Create an empty pixmap that is the same size as the PDF page. This
  // allows us to dielay the rendering of pages until they actually come into
  // view which saves time.
  QSizeF pageSize = page->pageSizeF() / 72.0;
  pageSize.setHeight(pageSize.height() * dpiY);
  pageSize.setWidth(pageSize.width() * dpiX);

  setPixmap(QPixmap(pageSize.toSize()));
}

void PDFPageGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
  // If this `PDFPageGraphicsItem` is still using the empty `QPixmap` it was
  // constructed with, `dirty` will be `true`. We replace the empty pixmap
  // with a rendered image of the page.
  if ( this->dirty ) {
    // `convertFromImage` was previously part of the Qt3 API and was
    // forward-ported to Qt 4 in version 4.7. Supposidly more efficient than
    // using `QPixmap::fromImage` as it does not involve constructing a new
    // QPixmap.
    //
    // If the performance hit is not outrageous, may want to consider
    // downgrading to `QPixmap::fromImage` in order to support more versions
    // of Qt 4.x.
    this->pixmap().convertFromImage(this->page->renderToImage(dpiX, dpiY));
    this->dirty = false;
  }

  // After checking if page image needs to be rendered, punt call back to the
  // `paint` method of `QGraphicsPixmapItem`.
  super::paint(painter, option, widget);
}


// PDFDocumentView
// ===============

// This class descends from `QGraphicsView` and is responsible for controlling
// and displaying the contents of a `Poppler::Document` using a `QGraphicsScene`.
PDFDocumentView::PDFDocumentView(Poppler::Document *a_doc, QWidget *parent) : super(new QGraphicsScene, parent),
  doc(a_doc),
  zoomLevel(1.0)
{
  setBackgroundRole(QPalette::Dark);
  setAlignment(Qt::AlignCenter);
  setFocusPolicy(Qt::StrongFocus);

  // **TODO:** _Investigate the Arthur backend for native Qt rendering._
  doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  doc->setRenderHint(Poppler::Document::Antialiasing);
  doc->setRenderHint(Poppler::Document::TextAntialiasing);

  _lastPage = doc->numPages();

  // Create a `PDFPageGraphicsItem` for each page in the PDF document to the
  // `QGraphicsScene` controlled by this object. The Y-coordinate of each
  // page is automatically shifted such that it will appear 10px below the
  // previous page.
  //
  // **TODO:** _Should the Y-shift be sensitive to zoom levels?_
  int i;
  float offY = 0.0;
  PDFPageGraphicsItem *pagePtr;

  for (i = 0; i < _lastPage; ++i) {
    pagePtr = new PDFPageGraphicsItem(doc->page(i));
    pagePtr->setPos(0.0, offY);

    pages.append(pagePtr);
    scene()->addItem(pagePtr);

    offY += pagePtr->pixmap().height() + 10.0;
  }

  // Automatically center on the first page in the PDF document.
  goToPage(0);
}

PDFDocumentView::~PDFDocumentView() {
  delete this->scene();
}

int PDFDocumentView::currentPage() {
  return _currentPage;
}

int PDFDocumentView::lastPage() {
  return _lastPage;
}

// **TODO:** _Overload this function to take `PDFPageGraphicsItem` as a
// parameter?_
void PDFDocumentView::goToPage(int pageNum) {
  // We silently ignore any invalid page numbers.
  if ( (pageNum >= 0) && (pageNum < _lastPage) && (pageNum != _currentPage) ) {
    centerOn(pages.at(pageNum));
    _currentPage = pageNum;
    emit changedPage(_currentPage);
  }
}

void PDFDocumentView::zoomIn() {
  zoomLevel *= 3.0/2.0;
  this->scale(3.0/2.0, 3.0/2.0);
  emit changedZoom(zoomLevel);
}

void PDFDocumentView::zoomOut() {
  zoomLevel *= 2.0/3.0;
  this->scale(2.0/3.0, 2.0/3.0);
  emit changedZoom(zoomLevel);
}

// Keep track of the current page by overloading the widget paint event.
void PDFDocumentView::paintEvent(QPaintEvent *event) {
  super::paintEvent(event);

  // After `QGraphicsView` has taken care of updates to this widget, find the
  // currently displayed page. We do this by grabbing all items that are
  // currently within the bounds of the viewport's top half. We take the
  // first item found to be the "current page".
  //
  // **NOTE:**
  // _If graphics objects other than `PDFPageGraphicsItem` are ever added to
  // the `GraphicsScene` managed by `PDFDocumentView` (such as annotations,
  // form elements, etc), it may be wise to ensure this selection only
  // considers `PDFPagegraphicsItem` objects.
  //
  // A way to do this may be to call `toSet` on both `pages` and the result
  // of `items` and then take the first item of a set intersection.
  QRect pageBbox = viewport()->rect();
  pageBbox.setHeight(0.5 * pageBbox.height());
  int nextCurrentPage = pages.indexOf(items(pageBbox).first());

  if (nextCurrentPage != _currentPage) {
    _currentPage = nextCurrentPage;
    emit changedPage(_currentPage);
  }

}

// Very "dumb" attempt at keypress handling.
//
// **TODO:**
//
//   * _Scroll events need to be captured and `currentPage` adjusted
//     accordingly._
//
//   * _Should pageUp/pageDown be implemented in terms of signals/slots and
//     let some parent widget worry about delegating Page Up/PageDown/other
//     keypresses?_
void PDFDocumentView::keyPressEvent(QKeyEvent *event) {

  if (
    (event->key() == Qt::Key_Home) &&
    (_currentPage != 0)
  ) {

    goToPage(0);
    event->accept();

  } else if(
    (event->key() == Qt::Key_End) &&
    (_currentPage != _lastPage)
  ) {

    goToPage(_lastPage - 1);
    event->accept();

  } else if(
    (event->key() == Qt::Key_PageDown) &&
    (_currentPage < _lastPage)
  ) {

    goToPage(_currentPage + 1);
    event->accept();

  } else if (
    (event->key() == Qt::Key_PageUp) &&
    (_currentPage > 0)
  ) {

    goToPage(_currentPage - 1);
    event->accept();

  } else  {

    super::keyPressEvent(event);

  }

}


PageCounter::PageCounter(QWidget *parent, Qt::WindowFlags f) : super(parent, f),
  currentPage(1),
  lastPage(-1)
{
  refreshText();
}

void PageCounter::setLastPage(int page){
  lastPage = page;
  refreshText();
}

void PageCounter::setCurrentPage(int page){
  // Assume the `SIGNAL` attached to this slot is emitting page numbers as
  // indicies in a 0-based array.
  currentPage = page + 1;
  refreshText();
}

void PageCounter::refreshText() {
  setText(QString("Page %1 of %2").arg(currentPage).arg(lastPage));
  update();
}


ZoomTracker::ZoomTracker(QWidget *parent, Qt::WindowFlags f) : super(parent, f),
  zoom(1.0)
{
  refreshText();
}

void ZoomTracker::setZoom(qreal newZoom){
  zoom = newZoom;
  refreshText();
}

void ZoomTracker::refreshText() {
  setText(QString("Zoom %1%").arg(zoom * 100));
  update();
}
