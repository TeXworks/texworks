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

// Some utility functions.
//
// **TODO:** _Find a better place to put these._
static bool isPageItem(QGraphicsItem *item) { return ( item->type() == PDFPageGraphicsItem::Type ); }

// PDFDocumentView
// ===============

// This class descends from `QGraphicsView` and is responsible for controlling
// and displaying the contents of a `Poppler::Document` using a `QGraphicsScene`.
//
// **TODO:**
// _This class basically comes with a built-in `QGraphicsScene` unlike a
// traditional `QGraphicsView` where the scenes can be swapped around. Should
// we disable the `setScene` function by declaring it `private`? Does it make
// sense to tightly couple a scene to this class?_
PDFDocumentView::PDFDocumentView(Poppler::Document *a_doc, QWidget *parent):
  Super(parent),
  zoomLevel(1.0)
{
  setBackgroundRole(QPalette::Dark);
  setAlignment(Qt::AlignCenter);
  setFocusPolicy(Qt::StrongFocus);

  // Having `pdf_scene` as a class member is a bit of a hack. Should probably
  // override or overload the `scene` method.
  pdf_scene = new PDFDocumentScene(a_doc, this);
  setScene(pdf_scene);
  _lastPage = pdf_scene->lastPage();

  // Respond to page jumps requested by the `PDFDocumentScene`.
  connect(pdf_scene, SIGNAL(pageChangeRequested(int)), this, SLOT(goToPage(int)));

  // Automatically center on the first page in the PDF document.
  goToPage(0);
}


// Accessors
// ---------
int PDFDocumentView::currentPage() { return _currentPage; }
int PDFDocumentView::lastPage()    { return _lastPage; }


// Public Slots
// ------------
void PDFDocumentView::goPrev()  { goToPage(_currentPage - 1); }
void PDFDocumentView::goNext()  { goToPage(_currentPage + 1); }
void PDFDocumentView::goFirst() { goToPage(0); }
void PDFDocumentView::goLast()  { goToPage(_lastPage - 1); }

// **TODO:** _Overload this function to take `PDFPageGraphicsItem` as a
// parameter?_
void PDFDocumentView::goToPage(int pageNum)
{
  // We silently ignore any invalid page numbers.
  if ( (pageNum >= 0) && (pageNum < _lastPage) && (pageNum != _currentPage) )
  {
    centerOn(pdf_scene->pages().at(pageNum));
    _currentPage = pageNum;
    emit changedPage(_currentPage);
  }
}


void PDFDocumentView::zoomIn()
{
  zoomLevel *= 3.0/2.0;
  this->scale(3.0/2.0, 3.0/2.0);
  emit changedZoom(zoomLevel);
}

void PDFDocumentView::zoomOut()
{
  zoomLevel *= 2.0/3.0;
  this->scale(2.0/3.0, 2.0/3.0);
  emit changedZoom(zoomLevel);
}


// Event Handlers
// --------------

// Keep track of the current page by overloading the widget paint event.
void PDFDocumentView::paintEvent(QPaintEvent *event)
{
  Super::paintEvent(event);

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
  int nextCurrentPage = pdf_scene->pageNumAt(mapToScene(pageBbox));

  if ( nextCurrentPage != _currentPage )
  {
    _currentPage = nextCurrentPage;
    emit changedPage(_currentPage);
  }

}

// **TODO:**
//
//   * _Should we let some parent widget worry about delegating Page
//     Up/PageDown/other keypresses?_
void PDFDocumentView::keyPressEvent(QKeyEvent *event)
{
  switch ( event->key() )
  {

    case Qt::Key_PageUp:
      goPrev();
      event->accept();
      break;

    case Qt::Key_PageDown:
      goNext();
      event->accept();
      break;

    case Qt::Key_Home:
      goFirst();
      event->accept();
      break;

    case Qt::Key_End:
      goLast();
      event->accept();
      break;

    default:
      Super::keyPressEvent(event);
      break;

  }
}


// PDFDocumentScene
// ================
PDFDocumentScene::PDFDocumentScene(Poppler::Document *a_doc, QObject *parent):
  Super(parent),
  doc(a_doc)
{
  // **TODO:** _Investigate the Arthur backend for native Qt rendering._
  doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  doc->setRenderHint(Poppler::Document::Antialiasing);
  doc->setRenderHint(Poppler::Document::TextAntialiasing);

  _lastPage = doc->numPages();

  // Create a `PDFPageGraphicsItem` for each page in the PDF document.  The
  // Y-coordinate of each page is shifted such that it will appear 10px below
  // the previous page.
  //
  // **TODO:** _Investigate things such as `QGraphicsGridLayout` that may take
  // care of offset for us and make it easy to extend to 2-up or multipage
  // views._
  int i;
  float offY = 0.0;
  PDFPageGraphicsItem *pagePtr;

  for (i = 0; i < _lastPage; ++i)
  {
    pagePtr = new PDFPageGraphicsItem(doc->page(i));
    pagePtr->setPos(0.0, offY);

    _pages.append(pagePtr);
    addItem(pagePtr);

    offY += pagePtr->pixmap().height() + 10.0;
  }
}


// Accessors
// ---------

QList<QGraphicsItem*> PDFDocumentScene::pages() { return _pages; };

// Overloaded method that returns all page objects inside a given rectangular
// area. First, `items` is used to grab all items inside the rectangle. This
// list is then filtered by item type so that it contains only references to
// `PDFPageGraphicsItem` objects.
QList<QGraphicsItem*> PDFDocumentScene::pages(const QPolygonF &polygon)
{
  QList<QGraphicsItem*> pageList = items(polygon);
  QtConcurrent::blockingFilter(pageList, isPageItem);

  return pageList;
};

// This is a convenience function for returning the page number of the first
// page item inside a given area of the scene.
int PDFDocumentScene::pageNumAt(const QPolygonF &polygon)
{
  return _pages.indexOf(pages(polygon).first());
}

int PDFDocumentScene::lastPage() { return _lastPage; }

// Event Handlers
// --------------

// We re-implement the main event handler for the scene so that we can
// translate events generated by child items into signals that can be sent out
// to the rest of the program.
bool PDFDocumentScene::event(QEvent *event)
{
  if ( event->type() == PDFLinkEvent::LinkEvent )
  {
    event->accept();
    // Cast to a pointer for `PDFLinkEvent` so that we can access the `pageNum`
    // field.
    const PDFLinkEvent *link_event = dynamic_cast<const PDFLinkEvent*>(event);

    // Translate into a signal that can be handled by some other part of the
    // program, such as a `PDFDocumentView`.
    emit pageChangeRequested(link_event->pageNum);
    return true;
  }

  return Super::event(event);
}

// PDFPageGraphicsItem
// ===================

// This class descends from `QGraphicsPixmapItem` and is responsible for
// rendering `Poppler::Page` objects.
//
// **TODO:**
// Currently the `pixmap` member inherited from `QGraphicsPixmapItem` is left
// blank since it determines this object's size and other geometric properties.
// The actual PDF pages are rendered to a new `QPixmap` object called
// `renderedPage`. This is a hack.
//
// `PDFPageGraphicsItem` should probably be re-implemented as a subclass of
// `QGraphicsItem` with custom methods for accessing geometry info.
// `QGraphicsObject` is another potential parent class and may be useful if
// images are ever rendered in a background thread as it provides support for
// `SIGNAL`/`SLOT` mechanics.
PDFPageGraphicsItem::PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent):
  Super(parent),
  page(a_page),
  dpiX(QApplication::desktop()->physicalDpiX()),
  dpiY(QApplication::desktop()->physicalDpiY()),

  dirty(true),
  zoomLevel(0.0)
{
  // Create an empty pixmap that is the same size as the PDF page. This
  // allows us to delay the rendering of pages until they actually come into
  // view yet still know what the page size is.
  QSizeF pageSize = page->pageSizeF() / 72.0;
  pageSize.setHeight(pageSize.height() * dpiY);
  pageSize.setWidth(pageSize.width() * dpiX);

  setPixmap(QPixmap(pageSize.toSize()));
}

int PDFPageGraphicsItem::type() const { return Type; }

// An overloaded paint method allows us to render the contents of
// `Poppler::Page` objects to `QImage` objects which are then stored inside the
// `PDFPageGraphicsItem` object using a `QPixmap`.
//
// **TODO:**
// _Page rendering and link loading should be handed off to a worker thread so
// that the GUI stays responsive._
void PDFPageGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Really, there is an X scaling factor and a Y scaling factor, but we assume
  // that the X scaling factor is equal to the Y scaling factor.
  qreal scaleFactor = painter->transform().m11();

  // If this is the first time this `PDFPageGraphicsItem` has come into view,
  // `dirty` will be `true`. We load all of the links on the page.
  if ( dirty )
  {
    createLinks(page->links());
    dirty = false;

    // **NOTE:**
    // _An update currently required to ensure links are drawn when the page
    // scrolls into view. This can probably be altered if the link creation is ever
    // shoved into a separate thread._
    update();
  }

  // We look at the zoom level and render a new page if it has changed or has
  // not been set yet.
  if ( zoomLevel != scaleFactor )
  {
    QRect bbox = painter->transform().mapRect(pixmap().rect());

    // We render to a new member named `renderedPage` rather than `pixmap`
    // because the properties of `pixmap` are used to calculate a bunch of size
    // and other geometric attributes in methods inherited from
    // `QGraphicsPixmapItem`.
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


void PDFPageGraphicsItem::createLinks(QList<Poppler::Link *> links) {
  // **TODO:**
  //
  //   * _Comment on how `pageTransform` works and is used._
  //
  //   * _Is this the best place to handle link <-> Qt graphics
  //     transformations?._
  QTransform pageTransform = QTransform::fromScale(pixmap().rect().width(), pixmap().rect().height());
  PDFLinkGraphicsItem *linkBox;
  foreach(Poppler::Link *link, links)
  {
    linkBox = new PDFLinkGraphicsItem(link, this);
    linkBox->setTransform(pageTransform);
  }
}


// PDFLinkGraphicsItem
// ===================

// This class descends from `QGraphicsRectItem` and serves the following
// functions:
//
//    * Provides easy access to the on-screen geometry of a hyperlink area.
//
//    * Handles tasks such as cursor changes on mouse hover and link activation
//      on mouse clicks.
PDFLinkGraphicsItem::PDFLinkGraphicsItem(Poppler::Link *a_link, QGraphicsItem *parent):
  Super(parent),
  _link(a_link),
  activated(false)
{
  // Poppler expresses the link area in "normalized page coordinates", i.e.
  // values in the range [0, 1]. The transformation matrix of this item will
  // have to be adjusted so that links will show up correctly in a graphics
  // view.
  setRect(_link->linkArea());

  // Allows links to provide a context-specific cursor when the mouse is
  // hovering over them.
  //
  // **NOTE:** _Requires Qt 4.4 or newer._
  setAcceptHoverEvents(true);

  // Only left-clicks will trigger the link.
  setAcceptedMouseButtons(Qt::LeftButton);

  // **TODO:**
  // _Intended to be for debugging purposes only so that the link area can be
  // determined visually_
  setPen(QPen(Qt::red));
}

int PDFLinkGraphicsItem::type() const { return Type; }

// Event Handlers
// --------------

// Swap cursor during hover events.
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

// The real nitty-gritty of link activation happens in here.
void PDFLinkGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check that this link was "activated" (mouse press occurred within the link
  // bounding box) and that the mouse release also occurred within the bounding
  // box.
  if ( (not activated) || (not contains(event->pos())) )
  {
    activated = false;
    return;
  }

  switch ( _link->linkType() )
  {

    case Poppler::Link::Goto:
    {
      const Poppler::LinkGoto *target = dynamic_cast<const Poppler::LinkGoto*>(_link);
      Q_ASSERT(target != NULL);

      // **FIXME:** _We don't handle this yet!_
      if ( target->isExternal() ) break;

      // Jump by page number. Links reckon page numbers starting with 1 so we
      // subtract to conform with 0-based indexing used by C++.
      //
      // **NOTE:**
      // _There are a many details that are not being considered, such as
      // centering on a specific anchor point and possibly changing the zoom
      // level rather than just focusing on the center of the target page._
      const int destPage = target->destination().pageNumber() - 1;

      // Post an event to the parent scene. The scene then takes care of
      // notifying objects, such as `PDFDocumentView`, that may want to take
      // action via a `SIGNAL`.
      QCoreApplication::postEvent(scene(), new PDFLinkEvent(destPage));
      break;
    }

    // Unsupported link types:
    //
    //     Poppler::Link::None
    //     Poppler::Link::Browse
    //     Poppler::Link::Execute
    //     Poppler::Link::JavaScript
    //     Poppler::Link::Action
    //     Poppler::Link::Sound
    //     Poppler::Link::Movie
    default:
      break;
  }

  activated = false;
}


// PDFLinkEvent
// ============

// A PDF Link event is generated when a link is clicked and contains the page
// number of the link target.
PDFLinkEvent::PDFLinkEvent(int a_page) : Super(LinkEvent), pageNum(a_page) {}

// Obtain a unique ID for `PDFLinkEvent` that can be used by event handlers to
// filter out these events.
QEvent::Type PDFLinkEvent::LinkEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );
