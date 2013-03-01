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
#include "PDFDocumentView.h"
#include <iostream>


// PDFPageGraphicsItem
// ===================

// This class descends from `QGraphicsPixmapItem` and is responsible for
// rendering `Poppler::Page` objects.
PDFPageGraphicsItem::PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent) : super(parent),
  page(a_page),
  dpiX(QApplication::desktop()->physicalDpiX()),
  dpiY(QApplication::desktop()->physicalDpiY()),

  dirty(true),
  zoomLevel(0.0)
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
  // Really, there is an X scaling factor and a Y scaling factor, but we assume
  // that the X scaling factor is equal to the Y scaling factor.
  qreal scaleFactor = painter->transform().m11();

  // If this is the first time this `PDFPageGraphicsItem` has come into view,
  // `dirty` will be `true`. We render the page and load all of its links.
  //
  // **TODO:** _The rendering and link loading should be handed off to a worker
  // thread so that the GUI stays responsive._
  if ( dirty ) {
    QRect bbox = painter->transform().mapRect(pixmap().rect());

    // We render to a new member named `renderedPage` rather than `pixmap`
    // because the properties of `pixmap` are used to calculate a bunch of size
    // and positioning information in methods inherited from
    // `QGraphicsPixmapItem`.
    renderedPage = QPixmap::fromImage(page->renderToImage(dpiX * scaleFactor, dpiY * scaleFactor,
      0, 0, bbox.width(), bbox.height()));

    // **TODO:**
    //   * _Comment on how exactly this transformation is valid._
    //   * _Is this the best place to handle transformations?._
    QTransform pageTransform = QTransform::fromScale(pixmap().rect().width(), pixmap().rect().height());
    PDFLinkGraphicsItem *linkBox;
    foreach(Poppler::Link *link, page->links()) {
      linkBox = new PDFLinkGraphicsItem(link, this);
      linkBox->setTransform(pageTransform);
    }

    // **NOTE:**
    // _An update currently required to ensure links are drawn when the page
    // scrolls into view. This can probably be altered if the link creation is ever
    // shoved into a seperate thread._
    update();

    zoomLevel = scaleFactor;
    dirty = false;
  }

  // We also look at the zoom level and render a new page if it has changed.
  //
  // **TODO:** _This operation should be seperated out into a method._
  if ( zoomLevel != scaleFactor ) {
    QRect bbox = painter->transform().mapRect(pixmap().rect());

    renderedPage = QPixmap::fromImage(page->renderToImage(dpiX * scaleFactor, dpiY * scaleFactor,
      0, 0, bbox.width(), bbox.height()));

    zoomLevel = scaleFactor;
  }

  // The transformation matrix of the `painter` object contains information
  // such as the current zoom level of the widget viewing this PDF page. We use
  // this matrix to position the page and then reset the transformation matrix
  // to an identity matrix as the page image has already been resized during
  // rendering.
  QPointF origin = painter->transform().map(offset());
  painter->setTransform(QTransform());

  // This part is modified from the `paint` method of `QGraphicsPixmapItem`.
  painter->setRenderHint(QPainter::SmoothPixmapTransform,
    (transformationMode() == Qt::SmoothTransformation));

  painter->drawPixmap(origin, renderedPage);
}


// PDFLinkGraphicsItem
// ===================

// This class descends from `QGraphicsRectItem` and serves as the on-screen
// representation of a PDF hyperlink area.
PDFLinkGraphicsItem::PDFLinkGraphicsItem(Poppler::Link *a_link, QGraphicsItem *parent) :
  super(parent),
  _link(a_link),
  activated(false)
{
  // The link area is expressed in "normalized page coordinates", i.e. values
  // in the range [0, 1]. The transformation matrix of this item will have to
  // be adjusted so that links will show up correctly in a graphics view.
  setRect(a_link->linkArea());

  // Allows links to provide a context-specific cursor when the mouse is
  // hovering over them.
  //
  // **NOTE:** Requires Qt 4.4 or newer.
  setAcceptHoverEvents(true);

  // Only left-clicks will trigger the link.
  setAcceptedMouseButtons(Qt::LeftButton);

  // Mainly for debugging purposes so that we can see where links are placed on
  // a page.
  setPen(QPen(Qt::red));
}

// Respond to hover events.
void PDFLinkGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  setCursor(Qt::PointingHandCursor);
}

void PDFLinkGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  unsetCursor();
}

// Respond to clicks. Limited to left-clicks by `setAcceptedMouseButtons` in
// this object's constructor.
void PDFLinkGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // Actually opening the link is handled during a `mouseReleaseEvent` --- but
  // only if the `activated` flag is `true`.
  activated = true;
}



void PDFLinkGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check that this link was "activated" (mouse press occurred within the link
  // bounding box) and that the mouse release also occurred within the bounding
  // box.
  if ( (not activated) || (not contains(event->pos())) ) {
    activated = false;
    return;
  }

  switch ( _link->linkType() ) {

    case Poppler::Link::Goto:
    {
      const Poppler::LinkGoto *target = dynamic_cast<const Poppler::LinkGoto*>(_link);
      Q_ASSERT(target != NULL);

      // **FIXME:** _We don't handle this yet!_
      if ( target->isExternal() ) break;

      // Jump by page number. Links reckon page numbers starting with 1 so we
      // subtract to conform with 0-based indexing used by C++.
      const int destPage = target->destination().pageNumber() - 1;

      // Don't actually jump as that requires some logic in other classes that
      // hasn't been implemented yet. Just display a little dialog showing the
      // page that would be jumped to.
      //
      // **TODO:** _Implement an actual change of page._
      QDialog *alert = new QDialog(0, Qt::Popup);
      QHBoxLayout *layout = new QHBoxLayout(alert);
      QTimer *timeout = new QTimer(alert);

      layout->addWidget(new QLabel(QString("Link to page %1").arg(destPage)));
      alert->setLayout(layout);

      QObject::connect(timeout, SIGNAL(timeout()), alert, SLOT(accept()));
      alert->show();
      timeout->start(1000);
      break;
    }

    default:
      // Unsupported link types:
      //
      //     Poppler::Link::None
      //     Poppler::Link::Browse
      //     Poppler::Link::Execute
      //     Poppler::Link::JavaScript
      //     Poppler::Link::Action
      //     Poppler::Link::Sound
      //     Poppler::Link::Movie
      break;
  }

  activated = false;
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


// Accessors
// ---------
int PDFDocumentView::currentPage() {
  return _currentPage;
}

int PDFDocumentView::lastPage() {
  return _lastPage;
}


// Public Slots
// ------------
void PDFDocumentView::goPrev() {
  goToPage(_currentPage - 1);
}

void PDFDocumentView::goNext() {
  goToPage(_currentPage + 1);
}

void PDFDocumentView::goFirst() {
  goToPage(0);
}

void PDFDocumentView::goLast() {
  goToPage(_lastPage - 1);
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


// Event Handlers
// --------------

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
  // considers `PDFPagegraphicsItem` objects._
  //
  // _A way to do this may be to call `toSet` on both `pages` and the result
  // of `items` and then take the first item of a set intersection._
  QRect pageBbox = viewport()->rect();
  pageBbox.setHeight(0.5 * pageBbox.height());
  int nextCurrentPage = pages.indexOf(items(pageBbox).first());

  if (nextCurrentPage != _currentPage) {
    _currentPage = nextCurrentPage;
    emit changedPage(_currentPage);
  }

}

// **TODO:**
//
//   * _Should we let some parent widget worry about delegating Page
//     Up/PageDown/other keypresses?_
void PDFDocumentView::keyPressEvent(QKeyEvent *event) {

  switch ( event->key() ) {

    case Qt::Key_PageUp:
      goPrev();
      break;

    case Qt::Key_PageDown:
      goNext();
      break;

    case Qt::Key_Home:
      goFirst();
      break;

    case Qt::Key_End:
      goLast();
      break;

    default:
      super::keyPressEvent(event);

  }

}
