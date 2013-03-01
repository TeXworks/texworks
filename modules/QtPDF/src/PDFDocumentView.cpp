/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
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

namespace QtPDF {

#ifdef DEBUG
#include <QDebug>
QTime stopwatch;
#endif

// Some utility functions.
//
// **TODO:** _Find a better place to put these._
static bool isPageItem(const QGraphicsItem *item) { return ( item->type() == PDFPageGraphicsItem::Type ); }

// PDFDocumentView
// ===============

// This class descends from `QGraphicsView` and is responsible for controlling
// and displaying the contents of a `Document` using a `QGraphicsScene`.
PDFDocumentView::PDFDocumentView(QWidget *parent):
  Super(parent),
  _pdf_scene(NULL),
  _zoomLevel(1.0),
  _useGrayScale(false),
  _pageMode(PageMode_OneColumnContinuous),
  _mouseMode(MouseMode_Move),
  _armedTool(NULL)
{
  setBackgroundRole(QPalette::Dark);
  setAlignment(Qt::AlignCenter);
  setFocusPolicy(Qt::StrongFocus);

  // If _currentPage is not set to -1, the compiler may default to 0. In that
  // case, `goFirst()` or `goToPage(0)` will fail because the view will think
  // it is already looking at page 0.
  _currentPage = -1;

  registerTool(new PDFDocumentToolZoomIn(this));
  registerTool(new PDFDocumentToolZoomOut(this));
  registerTool(new PDFDocumentToolMagnifyingGlass(this));
  registerTool(new PDFDocumentToolMarqueeZoom(this));
  registerTool(new PDFDocumentToolMove(this));
  registerTool(new PDFDocumentToolContextClick(this));

  // We deliberately set the mouse mode to a different value above so we can
  // call setMouseMode (which bails out if the mouse mode is not changed), which
  // in turn sets up other variables such as _toolAccessors
  setMouseMode(MouseMode_MagnifyingGlass);
  
  connect(&_searchResultWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(searchResultReady(int)));
  connect(&_searchResultWatcher, SIGNAL(progressValueChanged(int)), this, SLOT(searchProgressValueChanged(int)));
}

PDFDocumentView::~PDFDocumentView()
{
  if (!_searchResultWatcher.isFinished())
    _searchResultWatcher.cancel();
}

// Accessors
// ---------
void PDFDocumentView::setScene(QSharedPointer<PDFDocumentScene> a_scene)
{
  // FIXME: Make setScene(QGraphicsScene*) (from parent class) invisible to the
  // outside world
  Super::setScene(a_scene.data());

  // disconnect us from the old scene (if any)
  if (_pdf_scene) {
    disconnect(_pdf_scene.data(), 0, this, 0);
    _pdf_scene.clear();
  }

  _pdf_scene = a_scene;
  if (a_scene) {
    _lastPage = _pdf_scene->lastPage();
    // Respond to page jumps requested by the `PDFDocumentScene`.
    //
    // **TODO:**
    // _May want to consider not doing this by default. It is conceivable to have
    // a View that would ignore page jumps that other scenes would respond to._
    connect(_pdf_scene.data(), SIGNAL(pageChangeRequested(int)), this, SLOT(goToPage(int)));
    connect(_pdf_scene.data(), SIGNAL(pdfActionTriggered(const QtPDF::PDFAction*)), this, SLOT(pdfActionTriggered(const QtPDF::PDFAction*)));
    connect(_pdf_scene.data(), SIGNAL(documentChanged(const QSharedPointer<QtPDF::Document>)), this, SIGNAL(changedDocument(const QSharedPointer<QtPDF::Document>)));
  }
  else
    _lastPage = -1;
  
  // ensure the zoom is reset if we load a new document
  zoom100();

  // Ensure search result list is empty in case we are switching from another
  // scene.
  _searchResults.clear();
  if (_pdf_scene)
    emit changedDocument(_pdf_scene->document());
  else
    emit changedDocument(QSharedPointer<Document>());
}
int PDFDocumentView::currentPage() { return _currentPage; }
int PDFDocumentView::lastPage()    { return _lastPage; }

void PDFDocumentView::setPageMode(PageMode pageMode)
{
  if (!_pdf_scene || pageMode == _pageMode)
    return;

  QGraphicsItem *currentPage = _pdf_scene->pageAt(_currentPage);
  if (!currentPage)
    return;

  // Save the current view relative to the current page so we can restore it
  // after changing the mode
  // **TODO:** Safeguard
  QRectF viewRect(mapToScene(viewport()->rect()).boundingRect());
  viewRect.translate(-currentPage->pos());

  // **TODO:** Avoid relayouting everything twice when switching from SinglePage
  // to TwoColumnContinuous (once by setContinuous(), and a second time by
  // setColumnCount() below)
  switch (pageMode) {
    case PageMode_SinglePage:
      _pdf_scene->showOnePage(_currentPage);
      _pdf_scene->pageLayout().setContinuous(false);
      break;
    case PageMode_OneColumnContinuous:
      if (_pageMode == PageMode_SinglePage) {
        _pdf_scene->pageLayout().setContinuous(true);
        _pdf_scene->showAllPages();
        // Reset the scene rect; causes it the encompass the whole scene
        setSceneRect(QRectF());
      }
      _pdf_scene->pageLayout().setColumnCount(1, 0);
      break;
    case PageMode_TwoColumnContinuous:
      if (_pageMode == PageMode_SinglePage) {
        _pdf_scene->pageLayout().setContinuous(true);
        _pdf_scene->showAllPages();
        // Reset the scene rect; causes it the encompass the whole scene
        setSceneRect(QRectF());
      }
      _pdf_scene->pageLayout().setColumnCount(2, 1);
      break;
  }
  _pageMode = pageMode;
  _pdf_scene->pageLayout().relayout();

  // We might need to update the scene rect (when switching to single page mode)
  maybeUpdateSceneRect();

  // Restore the view from before as good as possible
  viewRect.translate(_pdf_scene->pageAt(_currentPage)->pos());
  ensureVisible(viewRect, 0, 0);
}

QDockWidget * PDFDocumentView::dockWidget(const Dock type, QWidget * parent /* = NULL */)
{
  QDockWidget * dock = new QDockWidget(QString(), parent);
  Q_ASSERT(dock != NULL);

  PDFDocumentInfoWidget * infoWidget;
  switch (type) {
    case Dock_TableOfContents:
    {
      PDFToCInfoWidget * tocWidget = new PDFToCInfoWidget(dock);
      connect(tocWidget, SIGNAL(actionTriggered(const QtPDF::PDFAction*)), this, SLOT(pdfActionTriggered(const QtPDF::PDFAction*)));
      infoWidget = tocWidget;
      break;
    }
    case Dock_MetaData:
      infoWidget = new PDFMetaDataInfoWidget(dock);
      break;
    case Dock_Fonts:
      infoWidget = new PDFFontsInfoWidget(dock);
      break;
    case Dock_Permissions:
      infoWidget = new PDFPermissionsInfoWidget(dock);
      break;
    case Dock_Annotations:
      infoWidget = new PDFAnnotationsInfoWidget(dock);
      // FIXME: possibility to jump to selected/activated annotation
      break;
    default:
      infoWidget = NULL;
      break;
  }
  if (!infoWidget) {
    dock->deleteLater();
    return NULL;
  }
  if (_pdf_scene) {
    if (_pdf_scene->document())
      infoWidget->initFromDocument(_pdf_scene->document());
    connect(this, SIGNAL(changedDocument(const QSharedPointer<QtPDF::Document>)), infoWidget, SLOT(initFromDocument(const QSharedPointer<QtPDF::Document>)));
  }
  dock->setWindowTitle(infoWidget->windowTitle());

  // We don't want docks to (need to) take up a lot of space. If the infoWidget
  // can't shrink, we thus put it into a scroll area that can
  if (!(infoWidget->sizePolicy().horizontalPolicy() & QSizePolicy::ShrinkFlag) || \
      !(infoWidget->sizePolicy().verticalPolicy() & QSizePolicy::ShrinkFlag)) {
    QScrollArea * scrollArea = new QScrollArea(dock);
    scrollArea->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    scrollArea->setWidget(infoWidget);
    dock->setWidget(scrollArea);
  }
  else
    dock->setWidget(infoWidget);
  return dock;
}

// Public Slots
// ------------

void PDFDocumentView::goPrev()  { goToPage(_currentPage - 1, Qt::AlignBottom); }
void PDFDocumentView::goNext()  { goToPage(_currentPage + 1, Qt::AlignTop); }
void PDFDocumentView::goFirst() { goToPage(0); }
void PDFDocumentView::goLast()  { goToPage(_lastPage - 1); }


// `goToPage` will shift the view to a different page. If the `alignment`
// parameter is `Qt::AlignLeft | Qt::AlignTop` (the default), the view will
// ensure the top left corner of the page is visible and aligned with the top
// left corner of the viewport (if possible). Other alignments can be used in
// the same way. If `alignment` for a direction is not set the view will
// show the same portion of the new page as it did before with the old page.
void PDFDocumentView::goToPage(const int pageNum, const int alignment /* = Qt::AlignLeft | Qt::AlignTop */)
{
  // We silently ignore any invalid page numbers.
  if (!_pdf_scene || pageNum < 0 || pageNum >= _lastPage)
    return;
  if (pageNum == _currentPage)
    return;

  goToPage((const PDFPageGraphicsItem*)_pdf_scene->pageAt(pageNum), alignment);
}

void PDFDocumentView::goToPage(const int pageNum, const QPointF anchor, const int alignment /* = Qt::AlignHCenter | Qt::AlignVCenter */)
{
  // We silently ignore any invalid page numbers.
  if (!_pdf_scene || pageNum < 0 || pageNum >= _lastPage)
    return;
  if (pageNum == _currentPage)
    return;

  goToPage((const PDFPageGraphicsItem*)_pdf_scene->pageAt(pageNum), anchor, alignment);
}

void PDFDocumentView::zoomBy(qreal zoomFactor)
{
  _zoomLevel *= zoomFactor;
  // Set the transformation anchor to AnchorViewCenter so we always zoom out of
  // the center of the view (rather than out of the upper left corner)
  QGraphicsView::ViewportAnchor anchor = transformationAnchor();
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  this->scale(zoomFactor, zoomFactor);
  setTransformationAnchor(anchor);

  emit changedZoom(_zoomLevel);
}

void PDFDocumentView::zoomIn() { zoomBy(3.0/2.0); }
void PDFDocumentView::zoomOut() { zoomBy(2.0/3.0); }

void PDFDocumentView::zoomToRect(QRectF a_rect)
{
  // NOTE: The argument, `a_rect`, is assumed to be in _scene coordinates_.
  fitInView(a_rect, Qt::KeepAspectRatio);

  // Since we passed `Qt::KeepAspectRatio` to `fitInView` both x and y scaling
  // factors were changed by the same amount. So we'll just take the x scale to
  // be the new `_zoomLevel`.
  _zoomLevel = transform().m11();
  emit changedZoom(_zoomLevel);
}

void PDFDocumentView::zoomFitWindow()
{
  if (!scene())
    return;

  QGraphicsItem *currentPage = _pdf_scene->pageAt(_currentPage);
  if (!currentPage)
    return;

  // Curious fact: This function will end up producing a different zoom level depending on if
  // it zooms out or in. But the implementation of `fitInView` in the Qt source
  // is pretty solid---I can't think of a better way to do it.
  fitInView(currentPage, Qt::KeepAspectRatio);

  _zoomLevel = transform().m11();
  emit changedZoom(_zoomLevel);
}


// `zoomFitWidth` is basically a re-worked version of `QGraphicsView::fitInView`.
void PDFDocumentView::zoomFitWidth()
{
  if ( !scene() || rect().isNull() )
      return;

  QGraphicsItem *currentPage = _pdf_scene->pageAt(_currentPage);
  if (!currentPage)
    return;
  
  // Store current y position so we can center on it later.
  qreal ypos = mapToScene(viewport()->rect()).boundingRect().center().y();

  // Reset the view scale to 1:1.
  QRectF unity = matrix().mapRect(QRectF(0, 0, 1, 1));
  if (unity.isEmpty())
      return;
  scale(1 / unity.width(), 1 / unity.height());

  // Find the x scaling ratio to fit the page to the view width.
  int margin = 2;
  QRectF viewRect = viewport()->rect().adjusted(margin, margin, -margin, -margin);
  if (viewRect.isEmpty())
      return;
  qreal xratio = viewRect.width() / currentPage->sceneBoundingRect().width();

  scale(xratio, xratio);
  // Focus on the horizontal center of the page and set the vertical position
  // to the previous y position.
  centerOn(QPointF(currentPage->sceneBoundingRect().center().x(), ypos));

  // We reset the scaling factors to (1,1) and then scaled both by the same
  // factor so the zoom level should be equal to the x scale.
  _zoomLevel = transform().m11();
  emit changedZoom(_zoomLevel);
}

void PDFDocumentView::zoom100()
{
  // Reset zoom level to 100%

  // Reset the view scale to 1:1.
  QRectF unity = matrix().mapRect(QRectF(0, 0, 1, 1));
  if (unity.isEmpty())
      return;

  // Set the transformation anchor to AnchorViewCenter so we always zoom out of
  // the center of the view (rather than out of the upper left corner)
  QGraphicsView::ViewportAnchor anchor = transformationAnchor();
  setTransformationAnchor(QGraphicsView::AnchorViewCenter);
  scale(1 / unity.width(), 1 / unity.height());
  setTransformationAnchor(anchor);

  _zoomLevel = transform().m11();
  emit changedZoom(_zoomLevel);
}

void PDFDocumentView::setMouseMode(const MouseMode newMode)
{
  if (_mouseMode == newMode)
    return;

  // TODO: eventually make _toolAccessors configurable
  _toolAccessors.clear();
  _toolAccessors[Qt::ControlModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_ContextClick);
  _toolAccessors[Qt::NoModifier + Qt::RightButton] = getToolByType(PDFDocumentTool::Tool_ContextMenu);
  _toolAccessors[Qt::NoModifier + Qt::MiddleButton] = getToolByType(PDFDocumentTool::Tool_Move);
  _toolAccessors[Qt::ShiftModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_ZoomIn);
  _toolAccessors[Qt::AltModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_ZoomOut);
  // Other tools: Tool_MagnifyingGlass, Tool_MarqueeZoom, Tool_Move

  disarmTool();

  switch (newMode) {
    case MouseMode_Move:
      armTool(PDFDocumentTool::Tool_Move);
      _toolAccessors[Qt::NoModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_Move);
      break;

    case MouseMode_MarqueeZoom:
      armTool(PDFDocumentTool::Tool_MarqueeZoom);
      _toolAccessors[Qt::NoModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_MarqueeZoom);
      break;

    case MouseMode_MagnifyingGlass:
      armTool(PDFDocumentTool::Tool_MagnifyingGlass);
      _toolAccessors[Qt::NoModifier + Qt::LeftButton] = getToolByType(PDFDocumentTool::Tool_MagnifyingGlass);
      break;
  }

  _mouseMode = newMode;
}

void PDFDocumentView::setMagnifierShape(const MagnifierShape shape)
{
  PDFDocumentToolMagnifyingGlass * magnifier = static_cast<PDFDocumentToolMagnifyingGlass*>(getToolByType(PDFDocumentTool::Tool_MagnifyingGlass));
  if (magnifier)
    magnifier->setMagnifierShape(shape);
}

void PDFDocumentView::setMagnifierSize(const int size)
{
  PDFDocumentToolMagnifyingGlass * magnifier = static_cast<PDFDocumentToolMagnifyingGlass*>(getToolByType(PDFDocumentTool::Tool_MagnifyingGlass));
  if (magnifier)
    magnifier->setMagnifierSize(size);
}

void PDFDocumentView::search(QString searchText)
{
  if ( not _pdf_scene )
    return;

  // If `searchText` is the same as for the last search, focus on the next 
  // search result.
  // Note: The primary use case for this is hitting `Enter` several times in the
  // search box to go to the next result.
  // On the other hand, with this there is no easy way to abort a long-running
  // search (e.g., in a large document) and restarting it in another part by
  // simply going there and hitting `Enter` again. As a workaround, one can
  // change the search text in that case (e.g., to something meaningless and
  // then back again to abort the previous search and restart at the new
  // location).
  if (searchText == _searchString) {
    nextSearchResult();
    return;
  }
  
  clearSearchResults();

  // Construct a list of requests that can be passed to QtConcurrent::mapped()
  QList<SearchRequest> requests;
  int i;
  for (i = _currentPage; i < _lastPage; ++i) {
    SearchRequest request;
    request.doc = _pdf_scene->document();
    request.pageNum = i;
    request.searchString = searchText;
    requests << request;
  }
  for (i = 0; i < _currentPage; ++i) {
    SearchRequest request;
    request.doc = _pdf_scene->document();
    request.pageNum = i;
    request.searchString = searchText;
    requests << request;
  }
  
  // If another search is still running, cancel it---after all, the user wants
  // to perform a new search
  if (!_searchResultWatcher.isFinished()) {
    _searchResultWatcher.cancel();
    _searchResultWatcher.waitForFinished();
  }

  _currentSearchResult = -1;
  _searchString = searchText;
  _searchResultWatcher.setFuture(QtConcurrent::mapped(requests, Page::search));
}

void PDFDocumentView::nextSearchResult()
{
  if ( not _pdf_scene || _searchResults.empty() )
    return;

  if ( (_currentSearchResult + 1) >= _searchResults.size() )
    _currentSearchResult = 0;
  else
    ++_currentSearchResult;

  centerOn(_searchResults[_currentSearchResult]);
}

void PDFDocumentView::previousSearchResult()
{
  if ( not _pdf_scene || _searchResults.empty() )
    return;

  if ( (_currentSearchResult - 1) < 0 )
    _currentSearchResult = _searchResults.size() - 1;
  else
    --_currentSearchResult;

  centerOn(_searchResults[_currentSearchResult]);
}

void PDFDocumentView::clearSearchResults()
{
  if ( not _pdf_scene || _searchResults.empty() )
    return;

  foreach( QGraphicsItem *item, _searchResults )
    _pdf_scene->removeItem(item);

  _searchResults.clear();
}


// Protected Slots
// --------------
void PDFDocumentView::searchResultReady(int index)
{
  // FIXME: The brush used for highlighting should be defined at global scope
  // to remove the need for re-creating it on each function call. Should also
  // be configurable via a settings object.
  QColor fillColor(Qt::yellow);
  fillColor.setAlphaF(0.6);
  QBrush highlightBrush(fillColor);

  // Convert the search result to highlight boxes
  foreach( SearchResult result, _searchResultWatcher.future().resultAt(index) ) {
    PDFPageGraphicsItem *page = qgraphicsitem_cast<PDFPageGraphicsItem*>(_pdf_scene->pageAt(result.pageNum));
    if (!page)
      continue;

    // This causes the page to take ownership of the highlight item which applies
    // necessary transformations and adds the item to the scene.
    QGraphicsRectItem *highlightItem = new QGraphicsRectItem(result.bbox, page);

    highlightItem->setBrush(highlightBrush);
    highlightItem->setPen(Qt::NoPen);
    highlightItem->setTransform(page->pointScale());

    _searchResults << highlightItem;
  }
  
  // If this is the first result that becomes available in a new search, center
  // on the first result
  if (_currentSearchResult == -1)
    nextSearchResult();
}

void PDFDocumentView::searchProgressValueChanged(int progressValue)
{
  // Inform the rest of the world of our progress (in %, and how many
  // occurrences were found so far)
  emit searchProgressChanged(100 * (progressValue - _searchResultWatcher.progressMinimum()) / (_searchResultWatcher.progressMaximum() - _searchResultWatcher.progressMinimum()), _searchResults.count());
}

void PDFDocumentView::maybeUpdateSceneRect() {
  if (!_pdf_scene || _pageMode != PageMode_SinglePage)
    return;

  // Set the scene rect of the view, i.e., the rect accessible via the scroll
  // bars. In single page mode, this must be the rect of the current page
  // **TODO:** Safeguard
  setSceneRect(_pdf_scene->pageAt(_currentPage)->sceneBoundingRect());
}

void PDFDocumentView::maybeArmTool(uint modifiers)
{
  // Arms the tool corresponding to `modifiers` if one is available. 
  PDFDocumentTool * t = _toolAccessors.value(modifiers, NULL);
  if (t != _armedTool) {
    disarmTool();
    armTool(t);
  }
}

void PDFDocumentView::goToPage(const PDFPageGraphicsItem * page, const int alignment /* = Qt::AlignLeft | Qt::AlignTop */)
{
  int pageNum;

  if (!_pdf_scene || !page || !isPageItem(page))
    return;
  pageNum = _pdf_scene->pageNumFor(page);
  if (pageNum == _currentPage)
    return;

  QRectF viewRect(mapToScene(QRect(QPoint(0, 0), viewport()->size())).boundingRect());

  // Note: This function must work if oldPage == NULL (e.g., during start up)
  PDFPageGraphicsItem *oldPage = (PDFPageGraphicsItem*)_pdf_scene->pageAt(_currentPage);
  if (oldPage && isPageItem(oldPage))
    viewRect = oldPage->mapRectFromScene(viewRect);
  else {
    // If we don't have an oldPage for whatever reason (e.g., during start up)
    // we default to the top left corner of newPage instead
    viewRect = page->mapRectFromScene(viewRect);
    viewRect.moveTopLeft(QPointF(0, 0));
  }

  switch (alignment & Qt::AlignHorizontal_Mask) {
    case Qt::AlignLeft:
      viewRect.moveLeft(page->boundingRect().left());
      break;
    case Qt::AlignRight:
      viewRect.moveRight(page->boundingRect().right());
      break;
    case Qt::AlignHCenter:
      viewRect.moveCenter(QPointF(page->boundingRect().center().x(), viewRect.center().y()));
      break;
    default:
      // without (valid) alignment, we don't do anything
      break;
  }
  switch (alignment & Qt::AlignVertical_Mask) {
    case Qt::AlignTop:
      viewRect.moveTop(page->boundingRect().top());
      break;
    case Qt::AlignBottom:
      viewRect.moveBottom(page->boundingRect().bottom());
      break;
    case Qt::AlignVCenter:
      viewRect.moveCenter(QPointF(viewRect.center().x(), page->boundingRect().center().y()));
      break;
    default:
      // without (valid) alignment, we don't do anything
      break;
  }

  if (_pageMode == PageMode_SinglePage) {
    _pdf_scene->showOnePage(page);
    maybeUpdateSceneRect();
  }

  viewRect = page->mapRectToScene(viewRect);
  // Note: ensureVisible seems to have a small glitch. Even if the passed
  // `viewRect` is identical, the result may depend on the view's previous state
  // if the margins are not -1. However, -1 margins don't work during the
  // initialization when the viewport doesn't have its final size yet (for
  // whatever reasons, the end result is a view centered on the scene).
  // So we use centerOn for now which should give the same result since
  // viewRect has the same size as the viewport.
//  ensureVisible(viewRect, -1, -1);
  centerOn(viewRect.center());

  _currentPage = pageNum;
  emit changedPage(_currentPage);
}

// TODO: Test
void PDFDocumentView::goToPage(const PDFPageGraphicsItem * page, const QPointF anchor, const int alignment /* = Qt::AlignHCenter | Qt::AlignVCenter */)
{
  int pageNum;

  if (!_pdf_scene || !page || !isPageItem(page))
    return;
  pageNum = _pdf_scene->pageNumFor(page);
  if (pageNum == _currentPage)
    return;

  QRectF viewRect(mapToScene(QRect(QPoint(0, 0), viewport()->size())).boundingRect());

  // Transform to item coordinates
  viewRect = page->mapRectFromScene(viewRect);

  switch (alignment & Qt::AlignHorizontal_Mask) {
    case Qt::AlignLeft:
      viewRect.moveLeft(anchor.x());
      break;
    case Qt::AlignRight:
      viewRect.moveRight(anchor.x());
      break;
    case Qt::AlignHCenter:
    default:
      viewRect.moveCenter(QPointF(anchor.x(), viewRect.center().y()));
      break;
  }
  switch (alignment & Qt::AlignVertical_Mask) {
    case Qt::AlignTop:
      viewRect.moveTop(anchor.y());
      break;
    case Qt::AlignBottom:
      viewRect.moveBottom(anchor.y());
      break;
    case Qt::AlignVCenter:
    default:
      viewRect.moveCenter(QPointF(viewRect.center().x(), anchor.y()));
      break;
  }

  if (_pageMode == PageMode_SinglePage) {
    _pdf_scene->showOnePage(page);
    maybeUpdateSceneRect();
  }

  viewRect = page->mapRectToScene(viewRect);
  // Note: ensureVisible seems to have a small glitch. Even if the passed
  // `viewRect` is identical, the result may depend on the view's previous state
  // if the margins are not -1. However, -1 margins don't work during the
  // initialization when the viewport doesn't have its final size yet (for
  // whatever reasons, the end result is a view centered on the scene).
  // So we use centerOn for now which should give the same result since
  // viewRect has the same size as the viewport.
//  ensureVisible(viewRect, -1, -1);
  centerOn(viewRect.center());

  _currentPage = pageNum;
  emit changedPage(_currentPage);
}

void PDFDocumentView::goToPage(const PDFPageGraphicsItem * page, const QRectF view, const bool mayZoom /* = false */)
{
  if (!page || page->page().isNull())
    return;

  // We must check if rect is valid, not view, as the latter usually has
  // negative height due to the inverted pdf coordinate system (y axis is up,
  // not down)
  QRectF rect(page->mapRectToScene(QRectF(page->mapFromPage(view.topLeft()), \
                                          page->mapFromPage(view.bottomRight()))));
  if (!rect.isValid())
    return;

  if (mayZoom) {
    fitInView(rect, Qt::KeepAspectRatio);
    _zoomLevel = transform().m11();
    emit changedZoom(_zoomLevel);
  }
  else
    centerOn(rect.center());

  if (_currentPage != page->page()->pageNum()) {
    _currentPage = page->page()->pageNum();
    emit changedPage(_currentPage);
  }
}

void PDFDocumentView::pdfActionTriggered(const PDFAction * action)
{
  if (!action)
    return;

  // Propagate link signals so that the outside world doesn't have to care about
  // our internal implementation (document/view structure, etc.)
  switch (action->type()) {
    case PDFAction::ActionTypeGoTo:
      {
        const PDFGotoAction * actionGoto = static_cast<const PDFGotoAction*>(action);
        // TODO: Possibly handle other properties of destination() (e.g.,
        // viewport settings, zoom level, etc.)
        // Note: if this action requires us to open other files (possible
        // security issue) or to create a new window, we need to propagate this
        // up the hierarchy. Otherwise we can handle it ourselves here.
        if (actionGoto->isRemote() || actionGoto->openInNewWindow())
          emit requestOpenPdf(actionGoto->filename(), actionGoto->destination().page(), actionGoto->openInNewWindow());
        else {
          Q_ASSERT(_pdf_scene != NULL);
          Q_ASSERT(!_pdf_scene->document().isNull());
          Q_ASSERT(isPageItem(_pdf_scene->pageAt(_currentPage)));
          PDFPageGraphicsItem * pageItem = static_cast<PDFPageGraphicsItem*>(_pdf_scene->pageAt(_currentPage));
          Q_ASSERT(pageItem != NULL);
          
          PDFDestination dest = _pdf_scene->document()->resolveDestination(actionGoto->destination());
          if (!dest.isValid() || !dest.isExplicit())
            break;

          // Get the current (=old) viewport in the current (=old) page's
          // coordinate system
          QRectF oldViewport = pageItem->mapRectFromScene(mapToScene(viewport()->rect()).boundingRect());
          oldViewport = QRectF(pageItem->mapToPage(oldViewport.topLeft()), \
                               pageItem->mapToPage(oldViewport.bottomRight()));
          // Calculate the new viewport (in page coordinates)
          QRectF view(dest.viewport(_pdf_scene->document().data(), oldViewport, _zoomLevel));

          goToPage(static_cast<PDFPageGraphicsItem*>(_pdf_scene->pageAt(dest.page())), view, true);
        }
      }
      break;
    case PDFAction::ActionTypeURI:
      {
        const PDFURIAction * actionURI = static_cast<const PDFURIAction*>(action);
        emit requestOpenUrl(actionURI->url());
      }
      break;
    case PDFAction::ActionTypeLaunch:
      {
        const PDFLaunchAction * actionLaunch = static_cast<const PDFLaunchAction*>(action);
        emit requestExecuteCommand(actionLaunch->command());
      }
      break;
    // **TODO:**
    // We don't handle other actions yet, but the ActionTypes Quit, Presentation,
    // EndPresentation, Find, GoToPage, Close, and Print should be propagated to
    // the outside world
    default:
      // All other link types are currently not supported
      break;
  }
}

void PDFDocumentView::registerTool(PDFDocumentTool * tool)
{
  int i;
  
  if (!tool)
    return;

  // Remove any identical tools
  for (i = 0; i < _tools.size(); ++i) {
    if (_tools[i] && *_tools[i] == *tool) {
      delete _tools[i];
      _tools.remove(i);
      --i;
    }
  }
  // Add the new tool
  _tools.append(tool);
}

PDFDocumentTool* PDFDocumentView::getToolByType(const PDFDocumentTool::Type type)
{
  foreach(PDFDocumentTool * tool, _tools) {
    if (tool && tool->type() == type)
      return tool;
  }
  return NULL;
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
  if (_pdf_scene) {
    QRect pageBbox = viewport()->rect();
    pageBbox.setHeight(0.5 * pageBbox.height());
    int nextCurrentPage = _pdf_scene->pageNumAt(mapToScene(pageBbox));

    if ( nextCurrentPage != _currentPage && nextCurrentPage >= 0 && nextCurrentPage < _lastPage )
    {
      _currentPage = nextCurrentPage;
      emit changedPage(_currentPage);
    }
  }

  if (_armedTool)
    _armedTool->paintEvent(event);
}

void PDFDocumentView::keyPressEvent(QKeyEvent *event)
{
  // FIXME: No moving while tools are active?
  switch ( event->key() )
  {
    case Qt::Key_Home:
      goFirst();
      event->accept();
      break;

    case Qt::Key_End:
      goLast();
      event->accept();
      break;

    case Qt::Key_PageUp:
    case Qt::Key_PageDown:
    case Qt::Key_Up:
    case Qt::Key_Down:
    case Qt::Key_Left:
    case Qt::Key_Right:
      // Check to see if we need to jump to the next page in single page mode.
      if ( pageMode() == PageMode_SinglePage ) {
        int scrollStep, scrollPos = verticalScrollBar()->value();

        if ( event->key() == Qt::Key_PageUp || event->key() == Qt::Key_PageDown )
          scrollStep = verticalScrollBar()->pageStep();
        else
          scrollStep = verticalScrollBar()->singleStep();


        // Take no action on the first and last page so that PageUp/Down can
        // move the view right up to the page boundary.
        if (
          (event->key() == Qt::Key_PageUp || event->key() == Qt::Key_Up) &&
          (scrollPos - scrollStep) <= verticalScrollBar()->minimum() &&
          _currentPage > 0
        ) {
          goPrev();
          event->accept();
          break;
        } else if (
          (event->key() == Qt::Key_PageDown || event->key() == Qt::Key_Down) &&
          (scrollPos + scrollStep) >= verticalScrollBar()->maximum() &&
          _currentPage < _lastPage
        ) {
          goNext();
          event->accept();
          break;
        }
      }

      // Deliberate fall-through; we only override the movement keys if a tool is
      // currently in use or the view is in single page mode and the movement
      // would cross a page boundary.
    default:
      Super::keyPressEvent(event);
      break;
  }
  // If we have an armed tool, pass the event on to it
  // Note: by default, PDFDocumentTool::keyPressEvent() calls maybeArmTool() if
  // it doesn't handle the event
  if (_armedTool)
    _armedTool->keyPressEvent(event);
  // If there is no currently armed tool, maybe we can arm one now
  else
    maybeArmTool(Qt::LeftButton + event->modifiers());
}

void PDFDocumentView::keyReleaseEvent(QKeyEvent *event)
{
  // If we have an armed tool, pass the event on to it
  // Note: by default, PDFDocumentTool::keyReleaseEvent() calls maybeArmTool() if
  // it doesn't handle the event
  if(_armedTool)
    _armedTool->keyReleaseEvent(event);
  else
    maybeArmTool(Qt::LeftButton + event->modifiers());
}

void PDFDocumentView::mousePressEvent(QMouseEvent * event)
{
  Super::mousePressEvent(event);

  // Don't do anything if the event was handled elsewhere (e.g., by a
  // PDFLinkGraphicsItem)
  if (event->isAccepted())
    return;

  PDFDocumentTool * oldArmed = _armedTool;
  
  if(_armedTool)
    _armedTool->mousePressEvent(event);
  else
    maybeArmTool(event->buttons() | event->modifiers());

  // This mouse event may have armed a new tool (either explicitly, or because
  // the previously armed tool passed it on to maybeArmTool). In that case, we
  // need to pass it on to the newly armed tool
  if (_armedTool && _armedTool != oldArmed)
    _armedTool->mousePressEvent(event);
}

void PDFDocumentView::mouseMoveEvent(QMouseEvent * event)
{
  // Note: to avoid reverting to _armed == Tool_None when moving the mouse
  // without pressing any button, we arm the default tool (corresponding to the
  // left mouse button) instead in that case
  Qt::MouseButtons buttons = event->buttons();
  if (buttons == Qt::NoButton)
    buttons = Qt::LeftButton;

  PDFDocumentTool * t = _toolAccessors.value(buttons | event->modifiers(), NULL);
  if (_armedTool != t) {
    disarmTool();
    armTool(t);
  }
  if(_armedTool)
    _armedTool->mouseMoveEvent(event);
  Super::mouseMoveEvent(event);

  // We don't check for event->isAccepted() here; for one, this always seems to
  // return true (for whatever reason), but more importantly, without enabling
  // mouse tracking we only receive this event if the current widget has grabbed
  // the mouse (i.e., after a mousePressEvent and before the corresponding
  // mouseReleaseEvent)
}

void PDFDocumentView::mouseReleaseEvent(QMouseEvent * event)
{
  Super::mouseReleaseEvent(event);

  // We don't check for event->isAccepted() here; for one, this always seems to
  // return true (for whatever reason), but more importantly, without enabling
  // mouse tracking we only receive this event if the current widget has grabbed
  // the mouse (i.e., after a mousePressEvent)

  Qt::MouseButtons buttons = event->buttons();
  if (buttons == Qt::NoButton)
    buttons |= Qt::LeftButton;

  PDFDocumentTool * t = _toolAccessors.value(buttons | event->modifiers(), NULL);
  if (_armedTool != t) {
    disarmTool();
    armTool(t);
  }
  if(_armedTool)
    _armedTool->mouseReleaseEvent(event);
}

void PDFDocumentView::wheelEvent(QWheelEvent * event)
{
  int delta = event->delta();

  if (event->orientation() == Qt::Vertical && event->buttons() == Qt::NoButton && event->modifiers() == Qt::ControlModifier) {

    // TODO: Possibly make the Ctrl modifier configurable?
    // TODO: According to Qt docs, the delta() is not necessarily the same for all
    // mice. Decide if we want to enforce the same step size regardless of the
    // resolution of the mouse wheel sensor
    if ( delta > 0 )
      zoomIn();
    else if ( delta < 0 )
      zoomOut();
    event->accept();
    return;

  } else if ( pageMode() == PageMode_SinglePage ) {

    // In single page mode we need to flip to the next page if the scroll bar
    // is a the top or bottom of it's range.`
    int scrollPos = verticalScrollBar()->value();
    if ( delta < 0 && scrollPos == verticalScrollBar()->maximum() ) {
      goNext();

      event->accept();
      return;
    } else if ( delta > 0 && scrollPos == verticalScrollBar()->minimum() ) {
      goPrev();

      event->accept();
      return;
    }

  }

  Super::wheelEvent(event);
}

void PDFDocumentView::armTool(const PDFDocumentTool::Type toolType)
{
  armTool(getToolByType(toolType));
}

void PDFDocumentView::armTool(PDFDocumentTool * tool)
{
  if (_armedTool == tool)
    return;
  if (_armedTool)
    disarmTool();
  if (tool)
    tool->arm();
  _armedTool = tool;
}

void PDFDocumentView::disarmTool()
{
  if (!_armedTool)
    return;
  _armedTool->disarm();
  _armedTool = NULL;
}


// PDFDocumentTool
// ========================
//
void PDFDocumentTool::arm() {
  Q_ASSERT(_parent != NULL);
  if (_parent->viewport())
    _parent->viewport()->setCursor(_cursor);
}
void PDFDocumentTool::disarm() {
  Q_ASSERT(_parent != NULL);
  if (_parent->viewport())
    _parent->viewport()->unsetCursor();
}

void PDFDocumentTool::keyPressEvent(QKeyEvent *event)
{
  if (_parent)
    _parent->maybeArmTool(Qt::LeftButton + event->modifiers());
}

void PDFDocumentTool::keyReleaseEvent(QKeyEvent *event)
{
  if (_parent)
    _parent->maybeArmTool(Qt::LeftButton + event->modifiers());
}

void PDFDocumentTool::mousePressEvent(QMouseEvent * event)
{
  if (_parent)
    _parent->maybeArmTool(event->buttons() | event->modifiers());
}

void PDFDocumentTool::mouseReleaseEvent(QMouseEvent * event)
{
  // If the last mouse button was released, we arm the tool corresponding to the
  // left mouse button by default
  Qt::MouseButtons buttons = event->buttons();
  if (buttons == Qt::NoButton)
    buttons |= Qt::LeftButton;

  if (_parent)
    _parent->maybeArmTool(buttons | event->modifiers());
}


// PDFDocumentToolZoomIn
// ========================
//
PDFDocumentToolZoomIn::PDFDocumentToolZoomIn(PDFDocumentView * parent)
: PDFDocumentTool(parent),
  _started(false)
{
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/zoomincursor.png")));
}

void PDFDocumentToolZoomIn::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started)
    _startPos = event->pos();
}

void PDFDocumentToolZoomIn::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPoint offset = event->pos() - _startPos;
    if (offset.manhattanLength() <  QApplication::startDragDistance())
      _parent->zoomIn();
  }
  _started = false;
}

// PDFDocumentToolZoomOut
// ========================
//
PDFDocumentToolZoomOut::PDFDocumentToolZoomOut(PDFDocumentView * parent)
: PDFDocumentTool(parent),
  _started(false)
{
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/zoomoutcursor.png")));
}

void PDFDocumentToolZoomOut::mousePressEvent(QMouseEvent * event)
{
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started)
    _startPos = event->pos();
}

void PDFDocumentToolZoomOut::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPoint offset = event->pos() - _startPos;
    if (offset.manhattanLength() <  QApplication::startDragDistance())
      _parent->zoomOut();
  }
  _started = false;
}


// PDFDocumentToolMagnifyingGlass
// ==============================
//
PDFDocumentToolMagnifyingGlass::PDFDocumentToolMagnifyingGlass(PDFDocumentView * parent) : 
  PDFDocumentTool(parent)
{
  _magnifier = new PDFDocumentMagnifierView(parent);
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/magnifiercursor.png")));
}

void PDFDocumentToolMagnifyingGlass::setMagnifierShape(const PDFDocumentView::MagnifierShape shape)
{
  Q_ASSERT(_magnifier != NULL);
  _magnifier->setShape(shape);
}

void PDFDocumentToolMagnifyingGlass::setMagnifierSize(const int size)
{
  Q_ASSERT(_magnifier != NULL);
  _magnifier->setSize(size);
}

void PDFDocumentToolMagnifyingGlass::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);

  if (_started) {
    _magnifier->prepareToShow();
    _magnifier->setPosition(event->pos());
  }
  _magnifier->setVisible(_started);

  // Ensure an update of the viewport so that the drop shadow is painted
  // correctly
  QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
  r.moveCenter(_magnifier->geometry().center());
  _parent->viewport()->update(r);
}

void PDFDocumentToolMagnifyingGlass::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;

  // Only update the portion of the viewport (possibly) obscured by the
  // magnifying glass and its shadow.
  QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
  r.moveCenter(_magnifier->geometry().center());
  _parent->viewport()->update(r);

  _magnifier->setPosition(event->pos());
}

void PDFDocumentToolMagnifyingGlass::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    _magnifier->hide();
    _started = false;
    // Force an update of the viewport so that the drop shadow is hidden
    QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
    r.moveCenter(_magnifier->geometry().center());
    _parent->viewport()->update(r);
  }
}

void PDFDocumentToolMagnifyingGlass::paintEvent(QPaintEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  Q_ASSERT(_parent != NULL);

  if (!_started)
    return;

  // Draw a drop shadow
  QPainter p(_parent->viewport());
  QPixmap& dropShadow(_magnifier->dropShadow());
  QRect r(QPoint(0, 0), dropShadow.size());
  r.moveCenter(_magnifier->geometry().center());
  p.drawPixmap(r.topLeft(), dropShadow);
}


// PDFDocumentToolMarqueeZoom
// ==========================
//
PDFDocumentToolMarqueeZoom::PDFDocumentToolMarqueeZoom(PDFDocumentView * parent) :
  PDFDocumentTool(parent)
{
  Q_ASSERT(_parent);
  _rubberBand = new QRubberBand(QRubberBand::Rectangle, _parent->viewport());
  _cursor = QCursor(Qt::CrossCursor);
}

void PDFDocumentToolMarqueeZoom::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  Q_ASSERT(_rubberBand != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started) {
    _startPos = event->pos();
    _rubberBand->setGeometry(QRect());
    _rubberBand->show();
  }
}

void PDFDocumentToolMarqueeZoom::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_rubberBand != NULL);

  QPoint o = _startPos, p = event->pos();

  if (event->buttons() != Qt::LeftButton ) {
    // The user somehow let go of the left button without us recieving an
    // event. Abort the zoom operation.
    _rubberBand->setGeometry(QRect());
    _rubberBand->hide();
  } else if ( (o - p).manhattanLength() > QApplication::startDragDistance() ) {
    // Update rubber band Geometry.
    _rubberBand->setGeometry(QRect(
      QPoint(qMin(o.x(),p.x()), qMin(o.y(), p.y())),
      QPoint(qMax(o.x(),p.x()), qMax(o.y(), p.y()))
    ));
  }
}

void PDFDocumentToolMarqueeZoom::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QRectF zoomRect = _parent->mapToScene(_rubberBand->geometry()).boundingRect();
    _rubberBand->hide();
    _rubberBand->setGeometry(QRect());
    _parent->zoomToRect(zoomRect);
  }
  _started = false;
}


// PDFDocumentToolMove
// ===================
//
PDFDocumentToolMove::PDFDocumentToolMove(PDFDocumentView * parent) :
  PDFDocumentTool(parent)
{
  _cursor = QCursor(Qt::OpenHandCursor);
  _closedHandCursor = QCursor(Qt::ClosedHandCursor);
}

void PDFDocumentToolMove::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started) {
    if (_parent->viewport())
      _parent->viewport()->setCursor(_closedHandCursor);
    _oldPos = event->pos();
  }
}

void PDFDocumentToolMove::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!_started || !event)
    return;

  // Adapted from <qt>/src/gui/graphicsview/qgraphicsview.cpp @ QGraphicsView::mouseMoveEvent
  QScrollBar *hBar = _parent->horizontalScrollBar();
  QScrollBar *vBar = _parent->verticalScrollBar();
  QPoint delta = event->pos() - _oldPos;
  hBar->setValue(hBar->value() - delta.x());
  vBar->setValue(vBar->value() - delta.y());
  _oldPos = event->pos();
}

void PDFDocumentToolMove::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton)
    if (_parent->viewport())
      _parent->viewport()->setCursor(_cursor);
  _started = false;
}


// PDFDocumentToolContextClick
// ===========================
//
void PDFDocumentToolContextClick::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
}

void PDFDocumentToolContextClick::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;

  _started = false;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPointF pos(_parent->mapToScene(event->pos()));
    QGraphicsItem * item = _parent->scene()->itemAt(pos);
    if (!item || item->type() != PDFPageGraphicsItem::Type)
      return;
    PDFPageGraphicsItem * pageItem = static_cast<PDFPageGraphicsItem*>(item);
    _parent->triggerContextClick(pageItem->page()->pageNum(), pageItem->mapToPage(pageItem->mapFromScene(pos)));
  }
}


// PDFDocumentMagnifierView
// ========================
//
PDFDocumentMagnifierView::PDFDocumentMagnifierView(PDFDocumentView *parent /* = 0 */) :
  Super(parent),
  _parent_view(parent),
  _zoomLevel(1.0),
  _zoomFactor(2.0),
  _shape(PDFDocumentView::Magnifier_Circle),
  _size(300)
{
  // the magnifier should initially be hidden
  hide();

  // suppress scrollbars
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  // suppress any border styling (which doesn't work with a mask, e.g., for a
  // circular magnifier)
  setFrameShape(QFrame::NoFrame);

  if (parent) {
    // transfer some settings from the parent view
    setBackgroundRole(parent->backgroundRole());
    setAlignment(parent->alignment());
  }

  setShape(_shape);
}

void PDFDocumentMagnifierView::prepareToShow()
{
  qreal zoomLevel;

  if (!_parent_view)
    return;

  // Ensure we have the same scene
  if (_parent_view->scene() != scene())
    setScene(_parent_view->scene());
  // Fix the zoom
  zoomLevel = _parent_view->zoomLevel() * _zoomFactor;
  if (zoomLevel != _zoomLevel)
    scale(zoomLevel / _zoomLevel, zoomLevel / _zoomLevel);
  _zoomLevel = zoomLevel;
  // Ensure we have enough padding at the border that we can display the
  // magnifier even beyond the edge
  setSceneRect(_parent_view->sceneRect().adjusted(-width() / _zoomLevel, -height() / _zoomLevel, width() / _zoomLevel, height() / _zoomLevel));
}

void PDFDocumentMagnifierView::setZoomFactor(const qreal zoomFactor)
{
  _zoomFactor = zoomFactor;
  // Actual handling of zoom levels happens in prepareToShow, as the zoom level
  // of the parent cannot change while the magnifier is shown
}

void PDFDocumentMagnifierView::setPosition(const QPoint pos)
{
  move(pos.x() - width() / 2, pos.y() - height() / 2);
  centerOn(_parent_view->mapToScene(pos));
}

void PDFDocumentMagnifierView::setShape(const PDFDocumentView::MagnifierShape shape)
{
  _shape = shape;

  // ensure the window rect is set properly for the new mode
  setSize(_size);

  switch (shape) {
    case PDFDocumentView::Magnifier_Rectangle:
      clearMask();
#ifdef Q_WS_MAC
      // On OS X there is a bug that affects masking of QAbstractScrollArea and
      // its subclasses:
      //
      //   https://bugreports.qt.nokia.com/browse/QTBUG-7150
      //
      // The fix is to explicitly mask the viewport. As of Qt 4.7.4, this bug
      // is still present.
      viewport()->clearMask();
#endif
      break;
    case PDFDocumentView::Magnifier_Circle:
      setMask(QRegion(rect(), QRegion::Ellipse));
#ifdef Q_WS_MAC
      // Hack to fix QTBUG-7150
      viewport()->setMask(QRegion(rect(), QRegion::Ellipse));
#endif
      break;
  }
  _dropShadow = QPixmap();
}

void PDFDocumentMagnifierView::setSize(const int size)
{
  _size = size;
  switch (_shape) {
    case PDFDocumentView::Magnifier_Rectangle:
      setFixedSize(size * 4 / 3, size);
      break;
    case PDFDocumentView::Magnifier_Circle:
      setFixedSize(size, size);
      break;
  }
  _dropShadow = QPixmap();
}

void PDFDocumentMagnifierView::paintEvent(QPaintEvent * event)
{
  Super::paintEvent(event);

  // Draw our custom border
  // Note that QGraphicsView is derived from QAbstractScrollArea, but we are not
  // asked to paint on that but on the widget it contains. Therefore, we can't
  // just say QPainter(this)
  QPainter painter(viewport());

  painter.setRenderHint(QPainter::Antialiasing);
  
  QPen pen(Qt::gray);
  pen.setWidth(2);
  
  QRect rect(this->rect());

  painter.setPen(pen);
  switch(_shape) {
    case PDFDocumentView::Magnifier_Rectangle:
      painter.drawRect(rect);
      break;
    case PDFDocumentView::Magnifier_Circle:
      // Ensure we're drawing where we should, regardless how the window system
      // handles masks
      painter.setClipRegion(mask());
      // **TODO:** It seems to be necessary to adjust the window rect by one pixel
      // to draw an evenly wide border; is there a better way?
      rect.adjust(1, 1, 0, 0);
      painter.drawEllipse(rect);
      break;
  }

  // **Note:** We don't/can't draw the drop-shadow here. The reason is that we
  // rely on Super::paintEvent to do the actual rendering, which constructs its
  // own QPainter so we can't do clipping. Resetting the mask is no option,
  // either, as that may trigger an update (recursion!).
  // Alternatively, we could fill the border with the background from the
  // underlying window. But _parent_view->render() no option, because it
  // requires QWidget::DrawChildren (apparently the QGraphicsItems are
  // implemented as child widgets) which would cause a recursion again (the
  // magnifier is also a child widget!). Calling scene()->render() is no option,
  // either, because then render requests for unmagnified images would originate
  // from here, which would break the current implementation of
  // PDFPageGraphicsItem::paint().
  // Instead, drop-shadows are drawn in PDFDocumentView::paintEvent(), invoking
  // PDFDocumentMagnifierView::dropShadow().
}

// Modelled after http://labs.qt.nokia.com/2009/10/07/magnifying-glass
QPixmap& PDFDocumentMagnifierView::dropShadow()
{
  if (!_dropShadow.isNull())
    return _dropShadow;

  int padding = 10;
  _dropShadow = QPixmap(width() + 2 * padding, height() + 2 * padding);

  _dropShadow.fill(Qt::transparent);

  switch(_shape) {
    case PDFDocumentView::Magnifier_Rectangle:
      {
        QPainterPath path;
        QRectF boundingRect(_dropShadow.rect().adjusted(0, 0, -1, -1));
        QLinearGradient gradient(boundingRect.center(), QPointF(0.0, boundingRect.center().y()));
        gradient.setSpread(QGradient::ReflectSpread);
        QGradientStops stops;
        QColor color(Qt::black);
        color.setAlpha(64);
        stops.append(QGradientStop(1.0 - padding * 2.0 / _dropShadow.width(), color));
        color.setAlpha(0);
        stops.append(QGradientStop(1.0, color));

        QPainter shadow(&_dropShadow);
        shadow.setRenderHint(QPainter::Antialiasing);

        // paint horizontal gradient
        gradient.setStops(stops);

        path = QPainterPath();
        path.moveTo(boundingRect.topLeft());
        path.lineTo(boundingRect.topLeft() + QPointF(padding, padding));
        path.lineTo(boundingRect.bottomRight() + QPointF(-padding, -padding));
        path.lineTo(boundingRect.bottomRight());
        path.lineTo(boundingRect.topRight());
        path.lineTo(boundingRect.topRight() + QPointF(-padding, padding));
        path.lineTo(boundingRect.bottomLeft() + QPointF(padding, -padding));
        path.lineTo(boundingRect.bottomLeft());
        path.closeSubpath();

        shadow.fillPath(path, gradient);

        // paint vertical gradient
        stops[0].first = 1.0 - padding * 2.0 / _dropShadow.height();
        gradient.setStops(stops);

        path = QPainterPath();
        path.moveTo(boundingRect.topLeft());
        path.lineTo(boundingRect.topLeft() + QPointF(padding, padding));
        path.lineTo(boundingRect.bottomRight() + QPointF(-padding, -padding));
        path.lineTo(boundingRect.bottomRight());
        path.lineTo(boundingRect.bottomLeft());
        path.lineTo(boundingRect.bottomLeft() + QPointF(padding, -padding));
        path.lineTo(boundingRect.topRight() + QPointF(-padding, padding));
        path.lineTo(boundingRect.topRight());
        path.closeSubpath();

        gradient.setFinalStop(QPointF(QRectF(_dropShadow.rect()).center().x(), 0.0));
        shadow.fillPath(path, gradient);
      }
      break;
    case PDFDocumentView::Magnifier_Circle:
      {
        QRadialGradient gradient(QRectF(_dropShadow.rect()).center(), _dropShadow.width() / 2.0, QRectF(_dropShadow.rect()).center());
        QColor color(Qt::black);
        color.setAlpha(0);
        gradient.setColorAt(1.0, color);
        color.setAlpha(64);
        gradient.setColorAt(1.0 - padding * 2.0 / _dropShadow.width(), color);
        
        QPainter shadow(&_dropShadow);
        shadow.setRenderHint(QPainter::Antialiasing);
        shadow.fillRect(_dropShadow.rect(), gradient);
      }
      break;
  }
  return _dropShadow;
}


// PDFDocumentScene
// ================
//
// A large canvas that manages the layout of QGraphicsItem subclasses. The
// primary items we are concerned with are PDFPageGraphicsItem and
// PDFLinkGraphicsItem.
PDFDocumentScene::PDFDocumentScene(Document *a_doc, QObject *parent):
  Super(parent),
  _doc(a_doc)
{
  Q_ASSERT(a_doc != NULL);
  // We need to register a QList<PDFLinkGraphicsItem *> meta-type so we can
  // pass it through inter-thread (i.e., queued) connections
  qRegisterMetaType< QList<PDFLinkGraphicsItem *> >();

  connect(&_pageLayout, SIGNAL(layoutChanged(const QRectF)), this, SLOT(pageLayoutChanged(const QRectF)));

  reinitializeScene();
}

void PDFDocumentScene::handleActionEvent(const PDFActionEvent * action_event)
{
  if (!action_event || !action_event->action)
    return;

  switch (action_event->action->type() )
  {
    // Link types that we don't handle here but that may be of interest
    // elsewhere (note: ActionGoto will be handled by
    // PDFDocumentView::pdfActionTriggered)
    case PDFAction::ActionTypeGoTo:
    case PDFAction::ActionTypeURI:
    case PDFAction::ActionTypeLaunch:
      break;
    default:
      // All other link types are currently not supported
      return;
  }
  // Translate into a signal that can be handled by some other part of the
  // program, such as a `PDFDocumentView`.
  emit pdfActionTriggered(action_event->action);
}


// Accessors
// ---------

QSharedPointer<Document> PDFDocumentScene::document() { return QSharedPointer<Document>(_doc); }
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

// Convenience function to avoid moving the complete list of pages around
// between functions if only one page is needed
QGraphicsItem* PDFDocumentScene::pageAt(const int idx)
{
  if (idx < 0 || idx >= _pages.size())
    return NULL;
  return _pages[idx];
}

// This is a convenience function for returning the page number of the first
// page item inside a given area of the scene. If no page is in the specified
// area, -1 is returned.
int PDFDocumentScene::pageNumAt(const QPolygonF &polygon)
{
  QList<QGraphicsItem*> p(pages(polygon));
  if (p.isEmpty())
    return -1;
  return _pages.indexOf(p.first());
}

int PDFDocumentScene::pageNumFor(const PDFPageGraphicsItem * const graphicsItem) const
{
  // Note: since we store QGraphicsItem* in _pages, we need to remove the const
  // or else indexOf() complains during compilation. Since we don't do anything
  // with the pointer, this should be safe to do while still remaining the
  // const'ness of `graphicsItem`, however.
  return _pages.indexOf(const_cast<PDFPageGraphicsItem * const>(graphicsItem));
}

int PDFDocumentScene::lastPage() { return _lastPage; }

// Event Handlers
// --------------

// We re-implement the main event handler for the scene so that we can
// translate events generated by child items into signals that can be sent out
// to the rest of the program.
bool PDFDocumentScene::event(QEvent *event)
{
  if ( event->type() == PDFActionEvent::ActionEvent )
  {
    event->accept();
    // Cast to a pointer for `PDFActionEvent` so that we can access the `pageNum`
    // field.
    const PDFActionEvent *action_event = static_cast<const PDFActionEvent*>(event);
    handleActionEvent(action_event);
    return true;
  }

  return Super::event(event);
}

// Public Slots
// --------------
void PDFDocumentScene::doUnlockDialog()
{
  Q_ASSERT(!_doc.isNull());

  bool ok;
  // TODO: Maybe use some parent for QInputDialog (and QMessageBox below)
  // instead of NULL?
  QString password = QInputDialog::getText(NULL, trUtf8("Unlock PDF"), trUtf8("Please enter the password to unlock the PDF"), QLineEdit::Password, QString(), &ok);
  if (ok) {
    if (_doc->unlock(password)) {
      // FIXME: the program crashes in the QGraphicsView::mouseReleaseEvent
      // handler (presumably from clicking the "Unlock" button) when
      // reinitializeScene() is called immediately. To work around this, delay
      // it until control returns to the event queue. Problem: slots connected
      // to documentChanged() will receive the new doc, but the scene itself
      // will not have changed, yet.
      QTimer::singleShot(1, this, SLOT(reinitializeScene()));
      // FIXME: Other parts of the program should connect to documentChanged
      // to update data (e.g., dock widgets, page number status bar widget, ...)
      emit documentChanged(_doc);
    }
    else
      QMessageBox::information(NULL, trUtf8("Incorrect password"), trUtf8("The password you entered was incorrect."));
  }
}

// Protected Slots
// --------------
void PDFDocumentScene::pageLayoutChanged(const QRectF& sceneRect)
{
  setSceneRect(sceneRect);
  emit pageLayoutChanged();
}

void PDFDocumentScene::reinitializeScene()
{
  clear();
  _lastPage = _doc->numPages();
  if (_doc->isLocked()) {
    // FIXME: Deactivate "normal" user interaction, e.g., zooming, panning, etc.
    QWidget * _unlockWidget = new QWidget();
    QVBoxLayout * layout = new QVBoxLayout();

    QLabel * lockIcon = new QLabel(_unlockWidget);
    lockIcon->setPixmap(QPixmap(QString::fromUtf8(":/icons/lock.png")));
    QLabel * lockText = new QLabel(tr("This document is locked. You need a password to open it."), _unlockWidget);
    QPushButton * lockButton = new QPushButton(tr("Unlock"), _unlockWidget);

    connect(lockButton, SIGNAL(clicked()), this, SLOT(doUnlockDialog()));

    layout->addWidget(lockIcon);
    layout->addWidget(lockText);
    layout->addSpacing(20);
    layout->addWidget(lockButton);

    layout->setAlignment(lockIcon, Qt::AlignHCenter);
    layout->setAlignment(lockText, Qt::AlignHCenter);
    layout->setAlignment(lockButton, Qt::AlignHCenter);

    _unlockWidget->setLayout(layout);
    addWidget(_unlockWidget);
  }
  else {
    // Create a `PDFPageGraphicsItem` for each page in the PDF document and let
    // them be layed out by a `PDFPageLayout` instance.
    int i;
    PDFPageGraphicsItem *pagePtr;

    for (i = 0; i < _lastPage; ++i)
    {
      pagePtr = new PDFPageGraphicsItem(_doc->page(i));
      _pages.append(pagePtr);
      addItem(pagePtr);
      _pageLayout.addPage(pagePtr);
    }
    _pageLayout.relayout();
  }
}


// Other
// -----
void PDFDocumentScene::showOnePage(const int pageIdx) const
{
  int i;

  for (i = 0; i < _pages.size(); ++i) {
    if (!isPageItem(_pages[i]))
      continue;
    _pages[i]->setVisible(i == pageIdx);
  }
}

void PDFDocumentScene::showOnePage(const PDFPageGraphicsItem * page) const
{
  int i;

  for (i = 0; i < _pages.size(); ++i) {
    if (!isPageItem(_pages[i]))
      continue;
    _pages[i]->setVisible(_pages[i] == page);
  }
}

void PDFDocumentScene::showAllPages() const
{
  int i;

  for (i = 0; i < _pages.size(); ++i) {
    if (!isPageItem(_pages[i]))
      continue;
    _pages[i]->setVisible(true);
  }
}


// PDFPageGraphicsItem
// ===================

// This class descends from `QGraphicsObject` and implements the on-screen
// representation of `Page` objects.
PDFPageGraphicsItem::PDFPageGraphicsItem(QSharedPointer<Page> a_page, QGraphicsItem *parent):
  Super(parent),
  _page(a_page),
  _dpiX(QApplication::desktop()->physicalDpiX()),
  _dpiY(QApplication::desktop()->physicalDpiY()),

  _linksLoaded(false),
  _annotationsLoaded(false),
  _zoomLevel(0.0)
{
  // Create an empty pixmap that is the same size as the PDF page. This
  // allows us to delay the rendering of pages until they actually come into
  // view yet still know what the page size is.
  _pageSize = _page->pageSizeF();
  _pageSize.setWidth(_pageSize.width() * _dpiX / 72.0);
  _pageSize.setHeight(_pageSize.height() * _dpiY / 72.0);

  // `_pageScale` holds a transformation matrix that can map between normalized
  // page coordinates (in the range 0...1) and the coordinate system for this
  // graphics item. `_pointScale` is similar, except it maps from coordinates
  // expressed in pixels at a resolution of 72 dpi.
  _pageScale = QTransform::fromScale(_pageSize.width(), _pageSize.height());
  _pointScale = QTransform::fromScale(_dpiX / 72.0, _dpiY / 72.0);

  // So we get information during paint events about what portion of the page
  // is visible.
  //
  // NOTE: This flag needs Qt 4.6 or newer.
  setFlags(QGraphicsItem::ItemUsesExtendedStyleOption);
}

QRectF PDFPageGraphicsItem::boundingRect() const { return QRectF(QPointF(0.0, 0.0), _pageSize); }
int PDFPageGraphicsItem::type() const { return Type; }

QPointF PDFPageGraphicsItem::mapFromPage(const QPointF & point) const
{
  // item coordinates are in pixels
  return QPointF(_pageSize.width() * point.x() / _page->pageSizeF().width(), \
    _pageSize.height() * (1.0 - point.y() / _page->pageSizeF().height()));
}

QPointF PDFPageGraphicsItem::mapToPage(const QPointF & point) const
{
  // item coordinates are in pixels
  return QPointF(_page->pageSizeF().width() * point.x() / _pageSize.width(), \
    _page->pageSizeF().height() * (1.0 - point.y() / _pageSize.height()));
}

// An overloaded paint method allows us to handle rendering via asynchronous
// calls to backend functions.
void PDFPageGraphicsItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  // Really, there is an X scaling factor and a Y scaling factor, but we assume
  // that the X scaling factor is equal to the Y scaling factor.
  qreal scaleFactor = painter->transform().m11();
  QTransform scaleT = QTransform::fromScale(scaleFactor, scaleFactor);
  QRect pageRect = scaleT.mapRect(boundingRect()).toAlignedRect(), pageTile;

  // If this is the first time this `PDFPageGraphicsItem` has come into view,
  // `_linksLoaded` will be `false`. We then load all of the links on the page.
  if ( not _linksLoaded )
  {
    _page->asyncLoadLinks(this);
    _linksLoaded = true;
  }
  
  if (!_annotationsLoaded) {
    // FIXME: Load annotations asynchronously?
    addAnnotations(_page->loadAnnotations());
    _annotationsLoaded = true;
  }

  if ( _zoomLevel != scaleFactor )
    _zoomLevel = scaleFactor;

  painter->save();
  // Clip to the exposed rectangle to prevent unnecessary drawing operations.
  // This can provide up to a 50% speedup depending on the size of the tile.
  painter->setClipRect(option->exposedRect);

  // The transformation matrix of the `painter` object contains information
  // such as the current zoom level of the widget viewing this PDF page. We
  // throw away the scaling information because that has already been
  // applied during page rendering. (Note: we don't support rotation/skewing,
  // so we only care about the translational part)
  QTransform pageT = painter->transform();
  painter->setTransform(QTransform::fromTranslate(pageT.dx(), pageT.dy()));

#ifdef DEBUG
  // Pen style used to draw the outline of each tile for debugging purposes.
  QPen tilePen(Qt::darkGray);
  tilePen.setStyle(Qt::DashDotLine);
  painter->setPen(tilePen);
#endif

  QRect visibleRect = scaleT.mapRect(option->exposedRect).toAlignedRect();
  QSharedPointer<QImage> renderedPage;

  int i, imin, imax;
  int j, jmin, jmax;

  imin = (visibleRect.left() - pageRect.left()) / TILE_SIZE;
  imax = (visibleRect.right() - pageRect.left());
  if (imax % TILE_SIZE == 0)
    imax /= TILE_SIZE;
  else
    imax = imax / TILE_SIZE + 1;

  jmin = (visibleRect.top() - pageRect.top()) / TILE_SIZE;
  jmax = (visibleRect.bottom() - pageRect.top());
  if (jmax % TILE_SIZE == 0)
    jmax /= TILE_SIZE;
  else
    jmax = jmax / TILE_SIZE + 1;

  for (j = jmin; j < jmax; ++j) {
    for (i = imin; i < imax; ++i) {
      QRect tile(i * TILE_SIZE, j * TILE_SIZE, TILE_SIZE, TILE_SIZE);

      bool useGrayScale = false;
      // If we are rendering a PDFDocumentView that has `useGrayScale` set
      // respect that setting.
      PDFDocumentView * view = (widget ? qobject_cast<PDFDocumentView*>(widget->parent()) : NULL);
      if (view && view->useGrayScale())
        useGrayScale = true;
      // If we are rendering a PDFDocumentMagnifierView who's parent
      // PDFDocumentView has `useGrayScale` set respect that setting.
      else if (widget && widget->parent() && widget->parent()->parent()) {
        PDFDocumentView * view = (widget ? qobject_cast<PDFDocumentView*>(widget->parent()->parent()) : NULL);
        if (view && view->useGrayScale())
          useGrayScale = true;
      }

      renderedPage = _page->getTileImage(this, _dpiX * scaleFactor, _dpiY * scaleFactor, tile);
      // we don't want a finished render thread to change our image while we
      // draw it
      _page->document()->pageCache().lock();
      // renderedPage as returned from getTileImage _should_ always be valid
      if ( renderedPage ) {
        if (useGrayScale) {
          // In gray scale mode, we need to obtain a deep copy of the rendered
          // page image to avoid altering the cached (color) image
          QImage postProcessed = renderedPage->copy();
          imageToGrayScale(postProcessed);
          painter->drawImage(tile.topLeft(), postProcessed);
        }
        else
          painter->drawImage(tile.topLeft(), *renderedPage);
      }
      _page->document()->pageCache().unlock();
#ifdef DEBUG
      painter->drawRect(tile);
#endif
    }
  }
  painter->restore();
}

//static
void PDFPageGraphicsItem::imageToGrayScale(QImage & img)
{
  // Casting to QRgb* only works for 32bit images
  Q_ASSERT(img.depth() == 32);
  QRgb * data = (QRgb*)img.scanLine(0);
  int i, gray;
  for (i = 0; i < img.byteCount() / 4; ++i) {
    // Qt formula (qGray()): 0.34375 * r + 0.5 * g + 0.15625 * b
    // MuPDF formula (rgb_to_gray()): r * 0.3f + g * 0.59f + b * 0.11f;
    gray = qGray(data[i]);
    data[i] = qRgba(gray, gray, gray, qAlpha(data[i]));
  }
}

// Event Handlers
// --------------
bool PDFPageGraphicsItem::event(QEvent *event)
{
  // Look for callbacks from asynchronous page operations.
  if( event->type() == PDFLinksLoadedEvent::LinksLoadedEvent ) {
    event->accept();

    // Cast to a `PDFLinksLoaded` event so we can access the links.
    const PDFLinksLoadedEvent *links_loaded_event = static_cast<const PDFLinksLoadedEvent*>(event);
    addLinks(links_loaded_event->links);

    return true;

  } else if( event->type() == PDFPageRenderedEvent::PageRenderedEvent ) {
    event->accept();

    // FIXME: We're sort of misusing the render event here---it contains a copy
    // of the image data that we never touch. The assumption is that the page
    // cache now has new data, so we call `update` to trigger a repaint which
    // fetches stuff from the cache.
    //
    // Perhaps there should be a separate event for when the cache is updated.
    update();

    return true;
  }

  // Otherwise, pass event to default handler.
  return Super::event(event);
}

// This method causes the `PDFPageGraphicsItem` to create `PDFLinkGraphicsItem`
// objects for a list of asynchronously generated `PDFLinkAnnotation` objects.
// The page item also takes ownership the objects created.  Calling
// `setParentItem` causes the link objects to be added to the scene that owns
// the page object. `update` is then called to ensure all links are drawn at
// once.
void PDFPageGraphicsItem::addLinks(QList< QSharedPointer<PDFLinkAnnotation> > links)
{
  PDFLinkGraphicsItem *linkItem;
#ifdef DEBUG
  stopwatch.start();
#endif
  foreach( QSharedPointer<PDFLinkAnnotation> link, links ){
    linkItem = new PDFLinkGraphicsItem(link);
    // Map the link from pdf coordinates to scene coordinates
    linkItem->setTransform(QTransform::fromTranslate(0, _pageSize.height()).scale(_dpiX / 72., -_dpiY / 72.));
    linkItem->setParentItem(this);
  }
#ifdef DEBUG
  qDebug() << "Added links in: " << stopwatch.elapsed() << " milliseconds";
#endif

  update();
}

void PDFPageGraphicsItem::addAnnotations(QList< QSharedPointer<PDFAnnotation> > annotations)
{
  PDFMarkupAnnotationGraphicsItem *markupAnnotItem;
#ifdef DEBUG
  stopwatch.start();
#endif
  foreach( QSharedPointer<PDFAnnotation> annot, annotations ){
    // We currently only handle popups
    if (!annot->isMarkup())
      continue;
    QSharedPointer<PDFMarkupAnnotation> markupAnnot = annot.staticCast<PDFMarkupAnnotation>();
    markupAnnotItem = new PDFMarkupAnnotationGraphicsItem(markupAnnot);
    // Map the link from pdf coordinates to scene coordinates
    markupAnnotItem->setTransform(QTransform::fromTranslate(0, _pageSize.height()).scale(_dpiX / 72., -_dpiY / 72.));
    markupAnnotItem->setParentItem(this);
  }
#ifdef DEBUG
  qDebug() << "Added annotations in: " << stopwatch.elapsed() << " milliseconds";
#endif

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
PDFLinkGraphicsItem::PDFLinkGraphicsItem(QSharedPointer<PDFLinkAnnotation> a_link, QGraphicsItem *parent):
  Super(parent),
  _link(a_link),
  _activated(false)
{
  // The link area is expressed in "normalized page coordinates", i.e.  values
  // in the range [0, 1]. The transformation matrix of this item will have to
  // be adjusted so that links will show up correctly in a graphics view.
  setRect(_link->rect());

  // Allows links to provide a context-specific cursor when the mouse is
  // hovering over them.
  //
  // **NOTE:** _Requires Qt 4.4 or newer._
  setAcceptHoverEvents(true);

  // Only left-clicks will trigger the link.
  setAcceptedMouseButtons(Qt::LeftButton);

#ifdef DEBUG
  // **TODO:**
  // _Currently for debugging purposes only so that the link area can be
  // determined visually, but might make a nice option._
  setPen(QPen(Qt::red));
#else
  // Perhaps there is a way to not draw the outline at all? Might be more
  // efficient...
  setPen(QPen(Qt::transparent));
#endif

  PDFAction * action = _link->actionOnActivation();
  if (action) {
    // Set some meaningful tooltip to inform the user what the link does
    // Using <p>...</p> ensures the tooltip text is interpreted as rich text
    // and thus is wrapping sensibly to avoid over-long lines.
    // Using PDFDocumentView::trUtf8 avoids having to explicitly derive
    // PDFLinkGraphicsItem explicily from QObject and puts all translatable
    // strings into the same context.
    switch(action->type()) {
      case PDFAction::ActionTypeGoTo:
        {
          PDFGotoAction * actionGoto = static_cast<PDFGotoAction*>(action);
          setToolTip(PDFDocumentView::trUtf8("<p>Goto page %1</p>").arg(actionGoto->destination().page() + 1));
        }
        break;
      case PDFAction::ActionTypeURI:
        {
          PDFURIAction * actionURI = static_cast<PDFURIAction*>(action);
          setToolTip(QString::fromUtf8("<p>%1</p>").arg(actionURI->url().toString()));
        }
        break;
      case PDFAction::ActionTypeLaunch:
        {
          PDFLaunchAction * actionLaunch = static_cast<PDFLaunchAction*>(action);
          setToolTip(PDFDocumentView::trUtf8("<p>Execute `%1`</p>").arg(actionLaunch->command()));
        }
        break;
      default:
        // All other link types are currently not supported
        break;
    }
  }
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

  // Post an event to the parent scene. The scene then takes care of processing
  // it further, notifying objects, such as `PDFDocumentView`, that may want to
  // take action via a `SIGNAL`.
  // **TODO:** Wouldn't a direct call be more efficient?
  if (_link && _link->actionOnActivation())
    QCoreApplication::postEvent(scene(), new PDFActionEvent(_link->actionOnActivation()));
  _activated = false;
}


// PDFMarkupAnnotationGraphicsItem
// ===============================

// This class descends from `QGraphicsRectItem` and serves the following
// functions:
//
//    * Provides easy access to the on-screen geometry of a markup annotation.
//
//    * Handles tasks such as cursor changes on mouse hover and link activation
//      on mouse clicks.
//
//    * Displays note popups if necessary
PDFMarkupAnnotationGraphicsItem::PDFMarkupAnnotationGraphicsItem(QSharedPointer<PDFMarkupAnnotation> annot, QGraphicsItem *parent):
  Super(parent),
  _annot(annot),
  _activated(false),
  _popup(NULL)
{
  // The area is expressed in "normalized page coordinates", i.e.  values
  // in the range [0, 1]. The transformation matrix of this item will have to
  // be adjusted so that links will show up correctly in a graphics view.
  setRect(_annot->rect());

  // Allows annotations to provide a context-specific cursor when the mouse is
  // hovering over them.
  //
  // **NOTE:** _Requires Qt 4.4 or newer._
  setAcceptHoverEvents(true);

  // Only left-clicks will trigger the popup (if any).
  setAcceptedMouseButtons(annot->popup() ? Qt::LeftButton : Qt::NoButton);

#ifdef DEBUG
  // **TODO:**
  // _Currently for debugging purposes only so that the annotation area can be
  // determined visually, but might make a nice option._
  setPen(QPen(Qt::blue));
#else
  // Perhaps there is a way to not draw the outline at all? Might be more
  // efficient...
  setPen(QPen(Qt::transparent));
#endif

  QString tooltip(annot->richContents());
  // If the text is not already split into paragraphs, we do that here to ensure
  // proper line folding in the tooltip and hence to avoid very wide tooltips.
  if (tooltip.indexOf(QString::fromAscii("<p>")) < 0)
    tooltip = QString::fromAscii("<p>%1</p>").arg(tooltip.replace(QChar::fromAscii('\n'), QString::fromAscii("</p>\n<p>")));
  setToolTip(tooltip);
}

int PDFMarkupAnnotationGraphicsItem::type() const { return Type; }

// Event Handlers
// --------------

// Swap cursor during hover events.
void PDFMarkupAnnotationGraphicsItem::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  if (_annot->popup())
    setCursor(Qt::PointingHandCursor);
}

void PDFMarkupAnnotationGraphicsItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  if (_annot->popup())
    unsetCursor();
}

// Respond to clicks. Limited to left-clicks by `setAcceptedMouseButtons` in
// this object's constructor.
void PDFMarkupAnnotationGraphicsItem::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // Actually opening the popup is handled during a `mouseReleaseEvent` --- but
  // only if the `_activated` flag is `true`.
  _activated = true;
}

void PDFMarkupAnnotationGraphicsItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *event)
{
  Q_ASSERT(event != NULL);

  if (!_activated)
    return;
  _activated = false;
  
  if (!contains(event->pos()) || !_annot)
    return;

  // Find widget that received this mouse event in the first place
  // Note: according to the Qt docs, QApplication::widgetAt() can be slow. But
  // we don't care here, as this is called only once.
  QWidget * sender = QApplication::widgetAt(event->screenPos());

  if (!sender || !qobject_cast<PDFDocumentView*>(sender->parent()))
    return;
  
  if (_popup) {
    if (_popup->isVisible())
      _popup->hide();
    else {
      _popup->move(sender->mapFromGlobal(event->screenPos()));
      _popup->show();
      _popup->raise();
      _popup->setFocus();
    }
    return;
  }
  
  _popup = new QWidget(sender);
  
  QStringList styles;
  if (_annot->color().isValid()) {
    QColor c(_annot->color());
    styles << QString::fromUtf8(".QWidget { background-color: %1; }").arg(c.name());
    if (qGray(c.rgb()) >= 100)
      styles << QString::fromUtf8(".QWidget, .QLabel { color: black; }");
    else
      styles << QString::fromUtf8(".QWidget, .QLabel { color: white; }");
  }
  else {
    styles << QString::fromUtf8(".QWidget { background-color: %1; }").arg(QApplication::palette().color(QPalette::Window).name());
      styles << QString::fromUtf8(".QWidget, .QLabel { color: %1; }").arg(QApplication::palette().color(QPalette::Text).name());
  }
  _popup->setStyleSheet(styles.join(QString::fromAscii("\n")));
  QGridLayout * layout = new QGridLayout(_popup);
  layout->setContentsMargins(2, 2, 2, 5);

  QLabel * subject = new QLabel(QString::fromUtf8("<b>%1</b>").arg(_annot->subject()), _popup);
  layout->addWidget(subject, 0, 0, 1, -1);
  QLabel * author = new QLabel(_annot->author(), _popup);
  layout->addWidget(author, 1, 0, 1, 1);
  QLabel * date = new QLabel(_annot->creationDate().toString(Qt::DefaultLocaleLongDate), _popup);
  layout->addWidget(date, 1, 1, 1, 1, Qt::AlignRight);
  QTextEdit * content = new QTextEdit(_annot->richContents(), _popup);
  content->setEnabled(false);
  layout->addWidget(content, 2, 0, 1, -1);

  _popup->setLayout(layout);
  _popup->move(sender->mapFromGlobal(event->screenPos()));
  _popup->show();
  // FIXME: Make popup closable, movable; position it properly (also upon 
  // zooming!), give some visible indication to which annotation it belongs.
  // (Probably turn it into a subclass of QWidget, too).
}

// PDFToCInfoWidget
// ============

PDFToCInfoWidget::PDFToCInfoWidget(QWidget * parent) :
  PDFDocumentInfoWidget(parent, PDFDocumentView::trUtf8("Table of Contents"))
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);

  _tree = new QTreeWidget(this);
  _tree->setAlternatingRowColors(true);
  _tree->setHeaderHidden(true);
  _tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  connect(_tree, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));

  layout->addWidget(_tree);
  setLayout(layout);
}

PDFToCInfoWidget::~PDFToCInfoWidget()
{
  clear();
}
  
void PDFToCInfoWidget::initFromDocument(const QSharedPointer<Document> doc)
{
  Q_ASSERT(_tree != NULL);
  const PDFToC data = doc->toc();
  clear();
  recursiveAddTreeItems(data, _tree->invisibleRootItem());
}

void PDFToCInfoWidget::clear()
{
  Q_ASSERT(_tree != NULL);
  recursiveClearTreeItems(_tree->invisibleRootItem());
}

void PDFToCInfoWidget::itemSelectionChanged()
{
  Q_ASSERT(_tree != NULL);
  // Since the ToC QTreeWidget is in single selection mode, the first element is
  // the only one.
  QTreeWidgetItem * item = _tree->selectedItems().first();
  Q_ASSERT(item != NULL);
  // TODO: It might be better to register PDFAction with the QMetaType framework
  // instead of doing casts with (void*).
  PDFAction * action = (PDFAction*)item->data(0, Qt::UserRole).value<void*>();
  if (action)
    emit actionTriggered(action);
}

//static
void PDFToCInfoWidget::recursiveAddTreeItems(const QList<PDFToCItem> & tocItems, QTreeWidgetItem * parentTreeItem)
{
  foreach (const PDFToCItem & tocItem, tocItems) {
    QTreeWidgetItem * treeItem = new QTreeWidgetItem(parentTreeItem, QStringList(tocItem.label()));
    treeItem->setForeground(0, tocItem.color());
    if (tocItem.flags()) {
      QFont font = treeItem->font(0);
      font.setBold(tocItem.flags().testFlag(PDFToCItem::Flag_Bold));
      font.setItalic(tocItem.flags().testFlag(PDFToCItem::Flag_Bold));
      treeItem->setFont(0, font);
    }
    treeItem->setExpanded(tocItem.isOpen());
    // TODO: It might be better to register PDFAction via QMetaType to avoid
    // having to use (void*).
    if (tocItem.action())
      treeItem->setData(0, Qt::UserRole, QVariant::fromValue((void*)tocItem.action()->clone()));

    // FIXME: page numbers in col 2, goto actions, etc.

    if (!tocItem.children().isEmpty())
      recursiveAddTreeItems(tocItem.children(), treeItem);
  }
}

//static
void PDFToCInfoWidget::recursiveClearTreeItems(QTreeWidgetItem * parent)
{
  Q_ASSERT(parent != NULL);
  while (parent->childCount() > 0) {
    QTreeWidgetItem * item = parent->child(0);
    recursiveClearTreeItems(item);
    PDFAction * action = (PDFAction*)item->data(0, Qt::UserRole).value<void*>();
    if (action)
      delete action;
    parent->removeChild(item);
    delete item;
  }
}


// PDFMetaDataInfoWidget
// ============
PDFMetaDataInfoWidget::PDFMetaDataInfoWidget(QWidget * parent) : 
  PDFDocumentInfoWidget(parent, PDFDocumentView::trUtf8("Meta Data"))
{
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // scrollArea ... the central widget of the QDockWidget
  // w ... the central widget of scrollArea
  // groupBox ... one (of many) group box in w
  // vLayout ... lays out the group boxes in w
  // layout ... lays out the actual data widgets in groupBox
  QVBoxLayout * vLayout = new QVBoxLayout(this);
  QGroupBox * groupBox;
  QFormLayout * layout;

  // We want the vLayout to set the size of w (which should encompass all child
  // widgets completely, since we in turn put it into scrollArea to handle
  // oversized children
  vLayout->setSizeConstraint(QLayout::SetFixedSize);
  // Set margins to 0 as space is very limited in the sidebar
  vLayout->setContentsMargins(0, 0, 0, 0);

  // The "Document" group box
  groupBox = new QGroupBox(PDFDocumentView::trUtf8("Document"), this);
  layout = new QFormLayout(groupBox);

  _title = new QLabel(groupBox);
  _title->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Title:"), _title);

  _author = new QLabel(groupBox);
  _author->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Author:"), _author);

  _subject = new QLabel(groupBox);
  _subject->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Subject:"), _subject);

  _keywords = new QLabel(groupBox);
  _keywords->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Keywords:"), _keywords);

  groupBox->setLayout(layout);
  vLayout->addWidget(groupBox);

  // The "Processing" group box
  groupBox = new QGroupBox(PDFDocumentView::trUtf8("Processing"), this);
  layout = new QFormLayout(groupBox);

  _creator = new QLabel(groupBox);
  _creator->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Creator:"), _creator);

  _producer = new QLabel(groupBox);
  _producer->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Producer:"), _producer);

  _creationDate = new QLabel(groupBox);
  _creationDate->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Creation date:"), _creationDate);

  _modDate = new QLabel(groupBox);
  _modDate->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Modification date:"), _modDate);

  _trapped = new QLabel(groupBox);
  _trapped->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
  layout->addRow(PDFDocumentView::trUtf8("Trapped:"), _trapped);

  groupBox->setLayout(layout);
  vLayout->addWidget(groupBox);

  // The "Other" group box
  _other = groupBox = new QGroupBox(PDFDocumentView::trUtf8("Other"), this);
  layout = new QFormLayout(groupBox);
  // Hide the "Other" group box unless it has something to display
  _other->setVisible(false);

  // Note: Items are added to the "Other" box dynamically in
  // initFromDocument()

  groupBox->setLayout(layout);
  vLayout->addWidget(groupBox);

  setLayout(vLayout);
}

void PDFMetaDataInfoWidget::initFromDocument(const QSharedPointer<Document> doc)
{
  if (!doc) {
    clear();
    return;
  }
  _title->setText(doc->title());
  _author->setText(doc->author());
  _subject->setText(doc->subject());
  _keywords->setText(doc->keywords());
  _creator->setText(doc->creator());
  _producer->setText(doc->producer());
  _creationDate->setText(doc->creationDate().toString(Qt::DefaultLocaleLongDate));
  _modDate->setText(doc->modDate().toString(Qt::DefaultLocaleLongDate));
  switch (doc->trapped()) {
    case Document::Trapped_True:
      _trapped->setText(PDFDocumentView::trUtf8("Yes"));
      break;
    case Document::Trapped_False:
      _trapped->setText(PDFDocumentView::trUtf8("No"));
      break;
    default:
      _trapped->setText(PDFDocumentView::trUtf8("Unknown"));
      break;
  }
  QFormLayout * layout = qobject_cast<QFormLayout*>(_other->layout());
  Q_ASSERT(layout != NULL);

  // Remove any items there may be
  while (layout->count() > 0) {
    QLayoutItem * child = layout->takeAt(0);
    if (child) {
      if (child->widget())
        child->widget()->deleteLater();
      delete child;
    }
  }
  QMap<QString, QString>::const_iterator it;
  for (it = doc->metaDataOther().constBegin(); it != doc->metaDataOther().constEnd(); ++it) {
    QLabel * l = new QLabel(it.value(), _other);
    l->setTextInteractionFlags((Qt::TextInteractionFlag)(Qt::TextSelectableByMouse | Qt::TextSelectableByKeyboard));
    layout->addRow(it.key(), l);
  }
  // Hide the "Other" group box unless it has something to display
  _other->setVisible(layout->count() > 0);
}

void PDFMetaDataInfoWidget::clear()
{
  _title->setText(QString());
  _author->setText(QString());
  _subject->setText(QString());
  _keywords->setText(QString());
  _creator->setText(QString());
  _producer->setText(QString());
  _creationDate->setText(QString());
  _modDate->setText(QString());
  _trapped->setText(PDFDocumentView::trUtf8("Unknown"));
  QFormLayout * layout = qobject_cast<QFormLayout*>(_other->layout());
  Q_ASSERT(layout != NULL);

  // Remove any items there may be
  while (layout->count() > 0) {
    QLayoutItem * child = layout->takeAt(0);
    if (child)
      delete child;
  }
}

// PDFFontsInfoWidget
// ============
PDFFontsInfoWidget::PDFFontsInfoWidget(QWidget * parent) :
  PDFDocumentInfoWidget(parent, PDFDocumentView::trUtf8("Fonts"))
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  _table = new QTableWidget(this);

#ifdef Q_WS_MAC /* don't do this on windows, as the font ends up too small */
  QFont f(_table->font());
  f.setPointSize(f.pointSize() - 2);
  _table->setFont(f);
#endif
  _table->setColumnCount(4);
  _table->setHorizontalHeaderLabels(QStringList() << PDFDocumentView::trUtf8("Name") << PDFDocumentView::trUtf8("Type") << PDFDocumentView::trUtf8("Subset") << PDFDocumentView::trUtf8("Source"));
  _table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _table->setAlternatingRowColors(true);
  _table->setShowGrid(false);
  _table->setSelectionBehavior(QAbstractItemView::SelectRows);
  _table->verticalHeader()->hide();
  _table->horizontalHeader()->setStretchLastSection(true);
  _table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

  layout->addWidget(_table);
  setLayout(layout);
}

void PDFFontsInfoWidget::initFromDocument(const QSharedPointer<Document> doc)
{
  Q_ASSERT(_table != NULL);

  clear();
  if (!doc)
    return;

  QList<PDFFontInfo> fonts = doc->fonts();
  _table->setRowCount(fonts.count());

  int i = 0;
  foreach (PDFFontInfo font, fonts) {
    _table->setItem(i, 0, new QTableWidgetItem(font.descriptor().pureName()));
    switch (font.fontType()) {
      case PDFFontInfo::FontType_Type0:
        _table->setItem(i, 1, new QTableWidgetItem(PDFDocumentView::trUtf8("Type 0")));
        break;
      case PDFFontInfo::FontType_Type1:
        _table->setItem(i, 1, new QTableWidgetItem(PDFDocumentView::trUtf8("Type 1")));
        break;
      case PDFFontInfo::FontType_MMType1:
        _table->setItem(i, 1, new QTableWidgetItem(PDFDocumentView::trUtf8("Type 1 (multiple master)")));
        break;
      case PDFFontInfo::FontType_Type3:
        _table->setItem(i, 1, new QTableWidgetItem(PDFDocumentView::trUtf8("Type 3")));
        break;
      case PDFFontInfo::FontType_TrueType:
        _table->setItem(i, 1, new QTableWidgetItem(PDFDocumentView::trUtf8("TrueType")));
        break;
    }
    _table->setItem(i, 2, new QTableWidgetItem(font.isSubset() ? PDFDocumentView::trUtf8("yes") : PDFDocumentView::trUtf8("no")));
    switch (font.source()) {
      case PDFFontInfo::Source_Embedded:
        _table->setItem(i, 3, new QTableWidgetItem(PDFDocumentView::trUtf8("[embedded]")));
        break;
      case PDFFontInfo::Source_Builtin:
        _table->setItem(i, 3, new QTableWidgetItem(PDFDocumentView::trUtf8("[builtin]")));
        break;
      case PDFFontInfo::Source_File:
        _table->setItem(i, 3, new QTableWidgetItem(font.fileName().canonicalFilePath()));
        break;
    }
    ++i;
  }
  _table->resizeColumnsToContents();
  _table->resizeRowsToContents();
  _table->sortItems(0);
}

void PDFFontsInfoWidget::clear()
{
  _table->clearContents();
  _table->setRowCount(0);
}


// PDFPermissionsInfoWidget
// ============
PDFPermissionsInfoWidget::PDFPermissionsInfoWidget(QWidget * parent) : 
  PDFDocumentInfoWidget(parent, PDFDocumentView::trUtf8("Permissions"))
{
  setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
  // layout ... lays out the widgets in w
  QFormLayout * layout = new QFormLayout(this);

  // We want the layout to set the size of w (which should encompass all child
  // widgets completely, since we in turn put it into scrollArea to handle
  // oversized children
  layout->setSizeConstraint(QLayout::SetFixedSize);

  _print = new QLabel(this);
  layout->addRow(PDFDocumentView::trUtf8("Printing:"), _print);
  _modify = new QLabel(this);
  layout->addRow(PDFDocumentView::trUtf8("Modifications:"), _modify);
  _extract = new QLabel(this);
  layout->addRow(PDFDocumentView::trUtf8("Extraction:"), _extract);
  _addNotes = new QLabel(this);
  layout->addRow(PDFDocumentView::trUtf8("Annotation:"), _addNotes);
  _form = new QLabel(this);
  layout->addRow(PDFDocumentView::trUtf8("Filling forms:"), _form);

  setLayout(layout);
}

void PDFPermissionsInfoWidget::initFromDocument(const QSharedPointer<Document> doc)
{
  if (!doc) {
    clear();
    return;
  }
  
  QFlags<Document::Permissions> & perm = doc->permissions();
  
  if (perm.testFlag(Document::Permission_Print)) {
    if (perm.testFlag(Document::Permission_PrintHighRes))
      _print->setText(PDFDocumentView::trUtf8("Allowed"));
    else
      _print->setText(PDFDocumentView::trUtf8("Low resolution only"));
  }
  else
    _print->setText(PDFDocumentView::trUtf8("Denied"));

  _modify->setToolTip(QString());
  if (perm.testFlag(Document::Permission_Change))
    _modify->setText(PDFDocumentView::trUtf8("Allowed"));
  else if (perm.testFlag(Document::Permission_Assemble)) {
    _modify->setText(PDFDocumentView::trUtf8("Assembling only"));
    _modify->setToolTip(PDFDocumentView::trUtf8("Insert, rotate, or delete pages and create bookmarks or thumbnail images"));
  }
  else
    _modify->setText(PDFDocumentView::trUtf8("Denied"));

  if (perm.testFlag(Document::Permission_Extract))
    _extract->setText(PDFDocumentView::trUtf8("Allowed"));
  else if (perm.testFlag(Document::Permission_ExtractForAccessibility))
    _extract->setText(PDFDocumentView::trUtf8("Accessibility support only"));
  else
    _extract->setText(PDFDocumentView::trUtf8("Denied"));

  if (perm.testFlag(Document::Permission_Annotate))
    _addNotes->setText(PDFDocumentView::trUtf8("Allowed"));
  else
    _addNotes->setText(PDFDocumentView::trUtf8("Denied"));

  if (perm.testFlag(Document::Permission_FillForm))
    _form->setText(PDFDocumentView::trUtf8("Allowed"));
  else
    _form->setText(PDFDocumentView::trUtf8("Denied"));
}

void PDFPermissionsInfoWidget::clear()
{
  _print->setText(PDFDocumentView::trUtf8("Denied"));
  _modify->setText(PDFDocumentView::trUtf8("Denied"));
  _extract->setText(PDFDocumentView::trUtf8("Denied"));
  _addNotes->setText(PDFDocumentView::trUtf8("Denied"));
  _form->setText(PDFDocumentView::trUtf8("Denied"));
}


// PDFAnnotationsInfoWidget
// ============
PDFAnnotationsInfoWidget::PDFAnnotationsInfoWidget(QWidget * parent) :
  PDFDocumentInfoWidget(parent, PDFDocumentView::trUtf8("Annotations"))
{
  QVBoxLayout * layout = new QVBoxLayout(this);
  layout->setContentsMargins(0, 0, 0, 0);
  _table = new QTableWidget(this);

#ifdef Q_WS_MAC /* don't do this on windows, as the font ends up too small */
  QFont f(_table->font());
  f.setPointSize(f.pointSize() - 2);
  _table->setFont(f);
#endif
  _table->setColumnCount(4);
  _table->setHorizontalHeaderLabels(QStringList() << PDFDocumentView::trUtf8("Page") << PDFDocumentView::trUtf8("Subject") << PDFDocumentView::trUtf8("Author") << PDFDocumentView::trUtf8("Contents"));
  _table->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
  _table->setEditTriggers(QAbstractItemView::NoEditTriggers);
  _table->setAlternatingRowColors(true);
  _table->setShowGrid(false);
  _table->setSelectionBehavior(QAbstractItemView::SelectRows);
  _table->verticalHeader()->hide();
  _table->horizontalHeader()->setStretchLastSection(true);
  _table->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);

  layout->addWidget(_table);
  setLayout(layout);
  
  connect(&_annotWatcher, SIGNAL(resultReadyAt(int)), this, SLOT(annotationsReady(int)));
}

void PDFAnnotationsInfoWidget::initFromDocument(const QSharedPointer<Document> doc)
{
  if (!doc)
    return;

  QList< QSharedPointer<Page> > pages;
  int i;
  for (i = 0; i < doc->numPages(); ++i) {
    QSharedPointer<Page> page = doc->page(i);
    if (page)
      pages << page;
  }
  
  // If another search is still running, cancel it---after all, the user wants
  // to perform a new search
  if (!_annotWatcher.isFinished()) {
    _annotWatcher.cancel();
    _annotWatcher.waitForFinished();
  }

  clear();
  _annotWatcher.setFuture(QtConcurrent::mapped(pages, PDFAnnotationsInfoWidget::loadAnnotations));
}

//static
QList< QSharedPointer<PDFAnnotation> > PDFAnnotationsInfoWidget::loadAnnotations(QSharedPointer<Page> page)
{
  if (!page)
    return QList< QSharedPointer<PDFAnnotation> >();
  return page->loadAnnotations();
}

void PDFAnnotationsInfoWidget::annotationsReady(int index)
{
  Q_ASSERT(_table != NULL);
  int i;
  
  i = _table->rowCount();
  _table->setRowCount(i + _annotWatcher.resultAt(index).count());


  foreach(QSharedPointer<PDFAnnotation> pdfAnnot, _annotWatcher.resultAt(index)) {
    // we only use valid markup annotation here
    if (!pdfAnnot || !pdfAnnot->isMarkup())
      continue;
    PDFMarkupAnnotation * annot = static_cast<PDFMarkupAnnotation*>(pdfAnnot.data());
    if (annot->page())
      _table->setItem(i, 0, new QTableWidgetItem(QString::number(annot->page()->pageNum() + 1)));
    _table->setItem(i, 1, new QTableWidgetItem(annot->subject()));
    _table->setItem(i, 2, new QTableWidgetItem(annot->author()));
    _table->setItem(i, 3, new QTableWidgetItem(annot->contents()));
    ++i;
  }
  _table->setRowCount(i);
}

void PDFAnnotationsInfoWidget::clear()
{
  _table->clearContents();
  _table->setRowCount(0);
}

// PDFActionEvent
// ============

// A PDF Link event is generated when a link is clicked and contains the page
// number of the link target.
PDFActionEvent::PDFActionEvent(const PDFAction * action) : Super(ActionEvent), action(action) {}

// Obtain a unique ID for `PDFActionEvent` that can be used by event handlers to
// filter out these events.
QEvent::Type PDFActionEvent::ActionEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );


PDFPageLayout::PDFPageLayout() :
_numCols(1),
_firstCol(0),
_xSpacing(10),
_ySpacing(10),
_isContinuous(true)
{
}

void PDFPageLayout::setColumnCount(const int numCols) {
  // We need at least one column, and we only handle changes
  if (numCols <= 0 || numCols == _numCols)
    return;

  _numCols = numCols;
  // Make sure the first column is still valid
  if (_firstCol >= _numCols)
    _firstCol = _numCols - 1;
  rearrange();
}

void PDFPageLayout::setColumnCount(const int numCols, const int firstCol) {
  // We need at least one column, and we only handle changes
  if (numCols <= 0 || (numCols == _numCols && firstCol == _firstCol))
    return;

  _numCols = numCols;

  if (firstCol < 0)
    _firstCol = 0;
  else if (firstCol >= _numCols)
    _firstCol = _numCols - 1;
  else
    _firstCol = firstCol;
  rearrange();
}

void PDFPageLayout::setFirstColumn(const int firstCol) {
  // We only handle changes
  if (firstCol == _firstCol)
    return;

  if (firstCol < 0)
    _firstCol = 0;
  else if (firstCol >= _numCols)
    _firstCol = _numCols - 1;
  else
    _firstCol = firstCol;
  rearrange();
}

void PDFPageLayout::setXSpacing(const qreal xSpacing) {
  if (xSpacing > 0)
    _xSpacing = xSpacing;
  else
    _xSpacing = 0.;
}

void PDFPageLayout::setYSpacing(const qreal ySpacing) {
  if (ySpacing > 0)
    _ySpacing = ySpacing;
  else
    _ySpacing = 0.;
}

void PDFPageLayout::setContinuous(const bool continuous /* = true */)
{
  if (continuous == _isContinuous)
    return;
  _isContinuous = continuous;
  if (!_isContinuous)
    setColumnCount(1, 0);
    // setColumnCount() calls relayout automatically
  else relayout();
}

int PDFPageLayout::rowCount() const {
  if (_layoutItems.isEmpty())
    return 0;
  return _layoutItems.last().row + 1;
}

void PDFPageLayout::addPage(PDFPageGraphicsItem * page) {
  LayoutItem item;

  if (!page)
    return;

  item.page = page;
  if (_layoutItems.isEmpty()) {
    item.row = 0;
    item.col = _firstCol;
  }
  else if (_layoutItems.last().col < _numCols - 1){
    item.row = _layoutItems.last().row;
    item.col = _layoutItems.last().col + 1;
  }
  else {
    item.row = _layoutItems.last().row + 1;
    item.col = 0;
  }
  _layoutItems.append(item);
}

void PDFPageLayout::removePage(PDFPageGraphicsItem * page) {
  QList<LayoutItem>::iterator it;
  int row, col;

  // **TODO:** Decide what to do with pages that are in the list multiple times
  // (see also insertPage())

  // First, find the page and remove it
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    if (it->page == page) {
      row = it->row;
      col = it->col;
      it = _layoutItems.erase(it);
      break;
    }
  }

  // Then, rearrange the pages behind it (no call to rearrange() to save time
  // by not going over the unchanged pages in front of the removed one)
  for (; it != _layoutItems.end(); ++it) {
    it->row = row;
    it->col = col;

    ++col;
    if (col >= _numCols) {
      col = 0;
      ++row;
    }
  }
}

void PDFPageLayout::insertPage(PDFPageGraphicsItem * page, PDFPageGraphicsItem * before /* = NULL */) {
  QList<LayoutItem>::iterator it;
  int row, col;
  LayoutItem item;

  item.page = page;

  // **TODO:** Decide what to do with pages that are in the list multiple times
  // (see also insertPage())

  // First, find the page to insert before and insert (row and col will be set
  // below)
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    if (it->page == before) {
      row = it->row;
      col = it->col;
      it = _layoutItems.insert(it, item);
      break;
    }
  }
  if (it == _layoutItems.end()) {
    // We haven't found "before", so we just append the page
    addPage(page);
    return;
  }

  // Then, rearrange the pages starting from the inserted one (no call to
  // rearrange() to save time by not going over the unchanged pages)
  for (; it != _layoutItems.end(); ++it) {
    it->row = row;
    it->col = col;

    ++col;
    if (col >= _numCols) {
      col = 0;
      ++row;
    }
  }
}

// Relayout the pages on the canvas
void PDFPageLayout::relayout() {
  if (_isContinuous)
    continuousModeRelayout();
  else
    singlePageModeRelayout();
}

// Relayout the pages on the canvas in continuous mode
void PDFPageLayout::continuousModeRelayout() {
  // Create arrays to hold offsets and make sure that they have
  // sufficient space (to avoid moving the data around in memory)
  QVector<qreal> colOffsets(_numCols + 1, 0), rowOffsets(rowCount() + 1, 0);
  int i;
  qreal x, y;
  QList<LayoutItem>::iterator it;
  PDFPageGraphicsItem * page;
  QSizeF pageSize;
  QRectF sceneRect;

  // First, fill the offsets with the respective widths and heights
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    if (!it->page || !it->page->_page)
      continue;
    page = it->page;
    pageSize = page->_page->pageSizeF();

    if (colOffsets[it->col + 1] < pageSize.width() * page->_dpiX / 72.)
      colOffsets[it->col + 1] = pageSize.width() * page->_dpiX / 72.;
    if (rowOffsets[it->row + 1] < pageSize.height() * page->_dpiY / 72.)
      rowOffsets[it->row + 1] = pageSize.height() * page->_dpiY / 72.;
  }

  // Next, calculate cumulative offsets (including spacing)
  for (i = 1; i <= _numCols; ++i)
    colOffsets[i] += colOffsets[i - 1] + _xSpacing;
  for (i = 1; i <= rowCount(); ++i)
    rowOffsets[i] += rowOffsets[i - 1] + _ySpacing;

  // Finally, position pages
  // **TODO:** Figure out why this loop causes some noticeable lag when switching
  // from SinglePage to continuous mode in a large document (but not when
  // switching between separate continuous modes)
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    if (!it->page || !it->page->_page)
      continue;
    // If we have more than one column, right-align the left-most column and
    // left-align the right-most column to avoid large space between columns
    // In all other cases, center the page in allotted space (in case we
    // stumble over pages of different sizes, e.g., landscape pages, etc.)
    pageSize = it->page->_page->pageSizeF();
    if (_numCols > 1 && it->col == 0)
      x = colOffsets[it->col + 1] - _xSpacing - pageSize.width() * page->_dpiX / 72.;
    else if (_numCols > 1 && it->col == _numCols - 1)
      x = colOffsets[it->col];
    else
      x = 0.5 * (colOffsets[it->col + 1] + colOffsets[it->col] - _xSpacing - pageSize.width() * page->_dpiX / 72.);
    // Always center the page vertically
    y = 0.5 * (rowOffsets[it->row + 1] + rowOffsets[it->row] - _ySpacing - pageSize.height() * page->_dpiY / 72.);
    it->page->setPos(x, y);
  }

  // leave some space around the pages (note that the space on the right/bottom
  // is already included in the corresponding Offset values)
  sceneRect.setRect(-_xSpacing, -_ySpacing, colOffsets[_numCols] + _xSpacing, rowOffsets[rowCount()] + _ySpacing);
  emit layoutChanged(sceneRect);
}

// Relayout the pages on the canvas in single page mode
void PDFPageLayout::singlePageModeRelayout()
{
  qreal width, height, maxWidth = 0.0, maxHeight = 0.0;
  QList<LayoutItem>::iterator it;
  PDFPageGraphicsItem * page;
  QSizeF pageSize;
  QRectF sceneRect;

  // We lay out all pages such that their center is in the origin (since only
  // one page is visible at any time, this is no problem)
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    if (!it->page || !it->page->_page)
      continue;
    page = it->page;
    pageSize = page->_page->pageSizeF();
    width = pageSize.width() * page->_dpiX / 72.;
    height = pageSize.height() * page->_dpiY / 72.;
    if (width > maxWidth)
      maxWidth = width;
    if (height > maxHeight)
      maxHeight = height;
    page->setPos(-width / 2., -height / 2.);
  }

  sceneRect.setRect(-maxWidth / 2., -maxHeight / 2., maxWidth, maxHeight);
  emit layoutChanged(sceneRect);
}

void PDFPageLayout::rearrange() {
  QList<LayoutItem>::iterator it;
  int row, col;

  row = 0;
  col = _firstCol;
  for (it = _layoutItems.begin(); it != _layoutItems.end(); ++it) {
    it->row = row;
    it->col = col;

    ++col;
    if (col >= _numCols) {
      col = 0;
      ++row;
    }
  }
}

} // namespace QtPDF

// vim: set sw=2 ts=2 et

