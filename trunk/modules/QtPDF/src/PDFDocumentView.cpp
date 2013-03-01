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
PDFDocumentView::PDFDocumentView(QWidget *parent):
  Super(parent),
  _zoomLevel(1.0)
{
  setBackgroundRole(QPalette::Dark);
  setAlignment(Qt::AlignCenter);
  setFocusPolicy(Qt::StrongFocus);

  // If _currentPage is not set to -1, the compiler may default to 0. In that
  // case, `goFirst()` or `goToPage(0)` will fail because the view will think
  // it is already looking at page 0.
  _currentPage = -1;
}


// Accessors
// ---------
void PDFDocumentView::setScene(PDFDocumentScene *a_scene)
{
  Super::setScene(a_scene);

  // **TODO:** _Replace with an overloaded `scene` method._
  _pdf_scene = a_scene;

  _lastPage = a_scene->lastPage();

  // Respond to page jumps requested by the `PDFDocumentScene`.
  //
  // **TODO:**
  // _May want to consider not doing this by default. It is conceivable to have
  // a View that would ignore page jumps that other scenes would respond to._
  connect(a_scene, SIGNAL(pageChangeRequested(int)), this, SLOT(goToPage(int)));
}
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
    centerOn(_pdf_scene->pages().at(pageNum));
    _currentPage = pageNum;
    emit changedPage(_currentPage);
  }
}


void PDFDocumentView::zoomIn()
{
  _zoomLevel *= 3.0/2.0;
  this->scale(3.0/2.0, 3.0/2.0);
  emit changedZoom(_zoomLevel);
}

void PDFDocumentView::zoomOut()
{
  _zoomLevel *= 2.0/3.0;
  this->scale(2.0/3.0, 2.0/3.0);
  emit changedZoom(_zoomLevel);
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
  int nextCurrentPage = _pdf_scene->pageNumAt(mapToScene(pageBbox));

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
//
// A large canvas that manages the layout of QGraphicsItem subclasses. The
// primary items we are concerned with are PDFPageGraphicsItem and
// PDFLinkGraphicsItem.
//
// The scene also holds a mutex which is used to serialize calls by child items
// (mostly PDFGraphicsPage) to the Poppler library as Poppler is not thread
// safe.
//
// This system may need to be re-worked because if another function somewhere
// accesses the Poppler document pointed to by `*a_doc` while the scene child
// items are executing tasks, we can produce a segfault. Because of this, the
// mutex may need to be held at a higher level.
PDFDocumentScene::PDFDocumentScene(Poppler::Document *a_doc, QObject *parent):
  Super(parent),
  _doc(a_doc),
  docMutex(new QMutex)
{
  // **TODO:** _Investigate the Arthur backend for native Qt rendering._
  _doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  _doc->setRenderHint(Poppler::Document::Antialiasing);
  _doc->setRenderHint(Poppler::Document::TextAntialiasing);

  _lastPage = _doc->numPages();

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
    pagePtr = new PDFPageGraphicsItem(_doc->page(i));
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
// `_renderedPage`. This is a hack.
//
// `PDFPageGraphicsItem` should probably be re-implemented as a subclass of
// `QGraphicsItem` with custom methods for accessing geometry info.
// `QGraphicsObject` is another potential parent class and may be useful if
// images are ever rendered in a background thread as it provides support for
// `SIGNAL`/`SLOT` mechanics.
PDFPageGraphicsItem::PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent):
  Super(parent),
  _page(a_page),
  _dpiX(QApplication::desktop()->physicalDpiX()),
  _dpiY(QApplication::desktop()->physicalDpiY()),

  _linksLoaded(false),
  _pageIsRendering(false),
  _zoomLevel(0.0)
{
  // The `_linkGenerator` is used to monitor asynchronous generation of
  // `PDFLinkGraphicsItem` objects associated with the links on this page.
  _linkGenerator = new QFutureWatcher< QList<PDFLinkGraphicsItem *> >(this);
  connect(_linkGenerator, SIGNAL(finished()), this, SLOT(addLinks()));

  // The `_pageImageGenerator` monitors asynchronous rendering jobs. Both
  // generator objects must acquire the same mutex from `PDFDocumentScene` as
  // Poppler is not thread safe.
  _pageImageGenerator = new QFutureWatcher<QImage>(this);
  connect(_pageImageGenerator, SIGNAL(finished()), this, SLOT(updateRenderedPage()));

  // Create an empty pixmap that is the same size as the PDF page. This
  // allows us to delay the rendering of pages until they actually come into
  // view yet still know what the page size is.
  QSizeF pageSize = _page->pageSizeF() / 72.0;
  pageSize.setHeight(pageSize.height() * _dpiY);
  pageSize.setWidth(pageSize.width() * _dpiX);

  _pageScale = QTransform::fromScale(pageSize.width(), pageSize.height());
  setPixmap(QPixmap(pageSize.toSize()));
  _renderedPage = QPixmap(pageSize.toSize());
}

int PDFPageGraphicsItem::type() const { return Type; }

// An overloaded paint method allows us to render the contents of
// `Poppler::Page` objects to `QImage` objects which are then stored inside the
// `PDFPageGraphicsItem` object using a `QPixmap`.
void PDFPageGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Really, there is an X scaling factor and a Y scaling factor, but we assume
  // that the X scaling factor is equal to the Y scaling factor.
  qreal scaleFactor = painter->transform().m11();

  // If this is the first time this `PDFPageGraphicsItem` has come into view,
  // `_linksLoaded` will be `false`. We then load all of the links on the page.
  if ( not _linksLoaded )
  {
    // If this page has links, we generate `PDFLinkGraphicsItems` in a separate
    // thread. The `_linkGenerator` will emit a `finished` signal when
    // generation is complete.
    _linkGenerator->setFuture(QtConcurrent::run(this, &PDFPageGraphicsItem::loadLinks));

    _linksLoaded = true;

    // This is a hack to give a nice white fill to pages that have not been
    // rendered. Would be nice to replace this with a "loading page" graphic.
    _renderedPage.fill();
  }

  // We look at the zoom level and render a new page if the zoom has changed or
  // still has the value of `0.0` set by the constructor.
  if ( (_zoomLevel != scaleFactor) && not _pageIsRendering )
  {
    // Dispatch page rendering to a new thread so that the GUI stays
    // responsive.
    _pageImageGenerator->setFuture(QtConcurrent::run(this, &PDFPageGraphicsItem::renderPage, scaleFactor));

    // Indicate that a render is in progress so that subsequent paint events
    // won't trigger a re-render. Once `_pageImageGenerator` emits a `finished`
    // signal, this boolean is cleared.
    _pageIsRendering = true;

    _zoomLevel = scaleFactor;
  }

  // The transformation matrix of the `painter` object contains information
  // such as the current zoom level of the widget viewing this PDF page. We use
  // this matrix to position the page and then reset the transformation matrix
  // to an identity matrix as the page image has already been resized during
  // rendering.
  QPointF origin = painter->transform().map(offset());

  // This part is modified from the `paint` method of `QGraphicsPixmapItem`.
  painter->setRenderHint(QPainter::SmoothPixmapTransform,
    (transformationMode() == Qt::SmoothTransformation));

  if ( _pageIsRendering ) {
    // A new resized page is still rendering, so we "blow up" our current
    // render and paint that.
    //
    // **TODO:** _The performance of this degrades heavily at high zoom levels.
    // Mostly due to the use of `scaled` on the pixmap. Find a more efficient
    // way to scale._
    QSizeF scaledSize = painter->transform().mapRect(boundingRect()).size();
    painter->setTransform(QTransform());
    painter->drawPixmap(origin, _renderedPage.scaled(scaledSize.toSize()));
  } else {
    painter->setTransform(QTransform());
    painter->drawPixmap(origin, _renderedPage);
  }
}


// Asynchronous Link Generation
// ----------------------------

// This function generates `PDFLinkGraphicsItem` objects. It is intended to be
// called asynchronously and so does not set parentage for the objects it
// generates --- this task is left to the `addLinks` method so that all the
// links are added and rendered in a synchronous operation.
QList<PDFLinkGraphicsItem *> PDFPageGraphicsItem::loadLinks()
{
  // **TODO:**
  //
  //   * _Comment on how `pageScale` works and is used._

  // We need to acquire a mutex from `PDFDocumentScene` as accessing page data,
  // such as reading link lists or rendering page images is not thread safe
  // among pages objects created from the same document object.
  QMutexLocker docLock(qobject_cast<PDFDocumentScene *>(scene())->docMutex);
    QList<Poppler::Link *> links = _page->links();
  docLock.unlock();

  QList<PDFLinkGraphicsItem *> linkList;
  if( links.empty() ) return linkList;

  PDFLinkGraphicsItem *linkItem;

  foreach( Poppler::Link *link, links )
  {
    linkItem = new PDFLinkGraphicsItem(link);
    linkItem->setTransform(_pageScale);

    linkList.append(linkItem);
  }

  return linkList;
}

// This method causes the `PDFPageGraphicsItem` to take ownership of
// asynchronously generated `PDFLinkGraphicsItem` objects. Calling
// `setParentItem` causes the link objects to be added to the scene that owns
// the page object. `update` is then called to ensure all links are drawn at
// once.
void PDFPageGraphicsItem::addLinks()
{
  foreach( PDFLinkGraphicsItem *item, _linkGenerator->result() ) item->setParentItem(this);

  update();
}


// Asynchronous Page Rendering
// ---------------------------

QImage PDFPageGraphicsItem::renderPage(qreal scaleFactor)
{
  // Rendering is not thread safe!
  QMutexLocker docLock(qobject_cast<PDFDocumentScene *>(scene())->docMutex);
    QImage pageRender = _page->renderToImage(_dpiX * scaleFactor, _dpiY * scaleFactor);
  docLock.unlock();

  return pageRender;
}

void PDFPageGraphicsItem::updateRenderedPage()
{
  // We store the rendered page in a new member named `renderedPage` rather
  // than the `pixmap` member inherited from `QGraphicsPixmapItem`. This is
  // because the size of `pixmap` is used to calculate a bunch of geometric
  // attributes for `QGraphicsPixmapItem`. When the page is re-rendered, we
  // just want to increase the resolution, not affect the geometry of the item
  // in the graphics scene.
  _renderedPage = QPixmap::fromImage(_pageImageGenerator->result());

  // Indicate that page rendering has completed and this item needs to be
  // re-drawn.
  _pageIsRendering = false;
  update();
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
  _activated(false)
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
  // only if the `_activated` flag is `true`.
  _activated = true;
}

// The real nitty-gritty of link activation happens in here.
void PDFLinkGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  // Check that this link was "activated" (mouse press occurred within the link
  // bounding box) and that the mouse release also occurred within the bounding
  // box.
  if ( (not _activated) || (not contains(event->pos())) )
  {
    _activated = false;
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
      // _There are many details that are not being considered, such as
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

  _activated = false;
}


// PDFLinkEvent
// ============

// A PDF Link event is generated when a link is clicked and contains the page
// number of the link target.
PDFLinkEvent::PDFLinkEvent(int a_page) : Super(LinkEvent), pageNum(a_page) {}

// Obtain a unique ID for `PDFLinkEvent` that can be used by event handlers to
// filter out these events.
QEvent::Type PDFLinkEvent::LinkEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );
