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
#ifndef PDFDocumentView_H
#define PDFDocumentView_H

#include <QtGui/QtGui>

#include <PDFBackend.h>

// Forward declare classes defined in this header.
class PDFDocumentScene;
class PDFPageGraphicsItem;
class PDFLinkGraphicsItem;
class PDFDocumentMagnifierView;
class PDFActionEvent;
class PDFToCDockWidget;
class PDFMetaDataDockWidget;
class PDFFontsDockWidget;

const int TILE_SIZE=1024;

class PDFDocumentView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Super;

  QSharedPointer<PDFDocumentScene> _pdf_scene;
  PDFDocumentMagnifierView * _magnifier;

  QRubberBand *_rubberBand;
  QPoint _rubberBandOrigin;
  QPoint _movePosition;

  qreal _zoomLevel;
  int _currentPage, _lastPage;

  QString _searchString;
  QList<QGraphicsItem *> _searchResults;
  int _currentSearchResult;

public:
  enum PageMode { PageMode_SinglePage, PageMode_OneColumnContinuous, PageMode_TwoColumnContinuous };
  enum MouseMode { MouseMode_MagnifyingGlass, MouseMode_Move, MouseMode_MarqueeZoom };
  enum Tool { Tool_None, Tool_MagnifyingGlass, Tool_ZoomIn, Tool_ZoomOut, Tool_MarqueeZoom, Tool_Move, Tool_ContextMenu, Tool_ContextClick };
  enum MagnifierShape { Magnifier_Rectangle, Magnifier_Circle };

  PDFDocumentView(QWidget *parent = 0);
  void setScene(QSharedPointer<PDFDocumentScene> a_scene);
  int currentPage();
  int lastPage();
  PageMode pageMode() const { return _pageMode; }
  qreal zoomLevel() const { return _zoomLevel; }

  // The ownership of the returned pointers is transferred to the caller (i.e.,
  // he has to destroy them, unless the `parent` widget does that automatically)
  // They are fully wired to this PDFDocumentView (e.g., clicking on entries in
  // the table of contents will change this view)
  QDockWidget * tocDockWidget(QWidget * parent);
  QDockWidget * metaDataDockWidget(QWidget * parent);
  QDockWidget * fontsDockWidget(QWidget * parent);

public slots:
  void goPrev();
  void goNext();
  void goFirst();
  void goLast();
  void goToPage(int pageNum, bool centerOnTop = true);
  void setPageMode(PageMode pageMode);
  void setSinglePageMode() { setPageMode(PageMode_SinglePage); }
  void setOneColContPageMode() { setPageMode(PageMode_OneColumnContinuous); }
  void setTwoColContPageMode() { setPageMode(PageMode_TwoColumnContinuous); }
  void setMouseMode(const MouseMode newMode);
  void setMouseModeMagnifyingGlass() { setMouseMode(MouseMode_MagnifyingGlass); }
  void setMouseModeMove() { setMouseMode(MouseMode_Move); }
  void setMouseModeMarqueeZoom() { setMouseMode(MouseMode_MarqueeZoom); }
  void setMagnifierShape(const MagnifierShape shape);
  void setMagnifierSize(const int size);

  void zoomBy(qreal zoomFactor);
  void zoomIn();
  void zoomOut();
  void zoomToRect(QRectF a_rect);
  void zoomFitWindow();
  void zoomFitWidth();

  void search(QString searchText);
  void nextSearchResult();
  void previousSearchResult();
  void clearSearchResults();

signals:
  void changedPage(int pageNum);
  void changedZoom(qreal zoomLevel);

  void requestOpenUrl(const QUrl url);
  void requestExecuteCommand(QString command);
  void requestOpenPdf(QString filename, int page, bool newWindow);
  void contextClick(const int page, const QPointF pos);

protected:
  // Keep track of the current page by overloading the widget paint event.
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void keyReleaseEvent(QKeyEvent *event);
  void mousePressEvent(QMouseEvent * event);
  void mouseMoveEvent(QMouseEvent * event);
  void mouseReleaseEvent(QMouseEvent * event);
  void wheelEvent(QWheelEvent * event);

  // Prepare to use a tool, e.g., by changing the mouse cursor; usually called
  // when the modifier keys are right so that a mousePressEvent of the left
  // mouse button will trigger startTool
  void armTool(const Tool tool);
  // Start using a tool - typically called from mousePressEvent
  void startTool(const Tool tool, QMouseEvent * event);
  // Finish using a tool - typically called from mouseReleaseEvent
  void finishTool(const Tool tool, QMouseEvent * event);
  // Abort using a tool - typically called from key*Event
  void abortTool(const Tool tool);
  // Counterpart to armTool; provided mainly for symmetry
  void disarmTool(const Tool tool);

protected slots:
  void maybeUpdateSceneRect();
  void pdfActionTriggered(const PDFAction * action);
  // Note: view specifies which part of the page should be visible and must
  // therefore be given in scene coordinates
  void goToPage(const PDFPageGraphicsItem * page, const QRectF view, const bool mayZoom = false);

private:
  PageMode _pageMode;
  MouseMode _mouseMode;
  QCursor _hiddenCursor;
  Tool _armedTool;
  Tool _activeTool;
  // Note: the uint key can be any combination of Qt::MouseButton and
  // Qt::KeyboardModifier (which use disjunct number ranges)
  QMap<uint, Tool> _toolAccessors;
  // Never try to set a vanilla QGraphicsScene, always use a PDFGraphicsScene.
  void setScene(QGraphicsScene *scene);
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFDocumentView)
};

class PDFDocumentMagnifierView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Super;

  PDFDocumentView * _parent_view;
  qreal _zoomLevel, _zoomFactor;

  PDFDocumentView::MagnifierShape _shape;
  int _size;

public:
  PDFDocumentMagnifierView(PDFDocumentView *parent = 0);
  // the zoom factor multiplies the parent view's _zoomLevel
  void setZoomFactor(const qreal zoomFactor);
  void setPosition(const QPoint pos);
  void setShape(const PDFDocumentView::MagnifierShape shape);
  void setSize(const int size);
  // ensures all settings are in sync with the parent view
  // make sure you call it before calling show()!
  // Note: we cannot override show() because prepareToShow() usually needs to be
  // called before setPosition as well (as it adjusts the region accessible in
  // setPosition())
  void prepareToShow();

  QPixmap& dropShadow();

protected:
  void wheelEvent(QWheelEvent * event) { event->ignore(); }
  void paintEvent(QPaintEvent * event);
  
  QPixmap _dropShadow;
};

class PDFToCDockWidget : public QDockWidget
{
  Q_OBJECT
public:
  PDFToCDockWidget(QWidget * parent);
  virtual ~PDFToCDockWidget();
  
  void setToCData(const PDFToC data);
signals:
  void actionTriggered(const PDFAction*);
private slots:
  void itemSelectionChanged();
private:
  void clearTree();
  static void recursiveAddTreeItems(const QList<PDFToCItem> & tocItems, QTreeWidgetItem * parentTreeItem);
  static void recursiveClearTreeItems(QTreeWidgetItem * parent);
};

class PDFMetaDataDockWidget : public QDockWidget
{
  Q_OBJECT
public:
  PDFMetaDataDockWidget(QWidget * parent);
  virtual ~PDFMetaDataDockWidget() { }
  
  void setMetaDataFromDocument(const QSharedPointer<Document> doc);
private:
  QLabel * _title;
  QLabel * _author;
  QLabel * _subject;
  QLabel * _keywords;
  QLabel * _creator;
  QLabel * _producer;
  QLabel * _creationDate;
  QLabel * _modDate;
  QLabel * _trapped;
  QGroupBox * _other;
};

class PDFFontsDockWidget : public QDockWidget
{
  Q_OBJECT
public:
  PDFFontsDockWidget(QWidget * parent);
  virtual ~PDFFontsDockWidget() { }
  
  void setFontsDataFromDocument(const QSharedPointer<Document> doc);
private:
  QTableWidget * _table;
};

// Cannot use QGraphicsGridLayout and similar classes for pages because it only
// works for QGraphicsLayoutItem (i.e., QGraphicsWidget)
class PDFPageLayout : public QObject {
  Q_OBJECT
  struct LayoutItem {
    PDFPageGraphicsItem * page;
    int row;
    int col;
  };

  QList<LayoutItem> _layoutItems;
  int _numCols;
  int _firstCol;
  qreal _xSpacing; // spacing in pixel @ zoom=1
  qreal _ySpacing;
  bool _isContinuous;

public:
  PDFPageLayout();
  virtual ~PDFPageLayout() { }
  int columnCount() const { return _numCols; }
  int firstColumn() const { return _firstCol; }
  int xSpacing() const { return _xSpacing; }
  int ySpacing() const { return _ySpacing; }
  bool isContinuous() const { return _isContinuous; }
  void setContinuous(const bool continuous = true);

  void setColumnCount(const int numCols);
  void setColumnCount(const int numCols, const int firstCol);
  void setFirstColumn(const int firstCol);
  void setXSpacing(const qreal xSpacing);
  void setYSpacing(const qreal ySpacing);
  int rowCount() const;

  void addPage(PDFPageGraphicsItem * page);
  void removePage(PDFPageGraphicsItem * page);
  void insertPage(PDFPageGraphicsItem * page, PDFPageGraphicsItem * before = NULL);

public slots:
  void relayout();

signals:
  void layoutChanged(const QRectF sceneRect);

private:
  void rearrange();
  void continuousModeRelayout();
  void singlePageModeRelayout();
};


class PDFDocumentScene : public QGraphicsScene
{
  Q_OBJECT
  typedef QGraphicsScene Super;

  const QSharedPointer<Document> _doc;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> _pages;
  int _lastPage;
  PDFPageLayout _pageLayout;
  void handleActionEvent(const PDFActionEvent * action_event);

public:
  PDFDocumentScene(Document *a_doc, QObject *parent = 0);

  QSharedPointer<Document> document();
  QList<QGraphicsItem*> pages();
  QList<QGraphicsItem*> pages(const QPolygonF &polygon);
  QGraphicsItem* pageAt(const int idx);
  int pageNumAt(const QPolygonF &polygon);
  int pageNumFor(PDFPageGraphicsItem * const graphicsItem) const;
  PDFPageLayout& pageLayout() { return _pageLayout; }

  void showOnePage(const int pageIdx) const;
  void showAllPages() const;

  int lastPage();

  const QSharedPointer<Document> document() const { return _doc; }

signals:
  void pageChangeRequested(int pageNum);
  void pageLayoutChanged();
  void pdfActionTriggered(const PDFAction * action);

protected slots:
  void pageLayoutChanged(const QRectF& sceneRect);

protected:
  bool event(QEvent* event);

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFDocumentScene)
};


// Inherits from `QGraphicsOject` instead of `QGraphicsItem` in order to
// support SIGNALS/SLOTS used by threaded rendering.
//
// NOTE: __`QGraphicsObject` was added in Qt 4.6__
class PDFPageGraphicsItem : public QGraphicsObject
{
  Q_OBJECT
  typedef QGraphicsObject Super;

  QSharedPointer<Page> _page;

  double _dpiX;
  double _dpiY;
  QSizeF _pageSize;

  bool _linksLoaded;

  QTransform _pageScale, _pointScale;
  qreal _zoomLevel;

  friend class PageProcessingRenderPageRequest;
  friend class PageProcessingLoadLinksRequest;
  friend class PDFPageLayout;

public:

  PDFPageGraphicsItem(QSharedPointer<Page> a_page, QGraphicsItem *parent = 0);

  // This seems fragile as it assumes no other code declaring a custom graphics
  // item will choose the same ID for it's object types. Unfortunately, there
  // appears to be no equivalent of `registerEventType` for `QGraphicsItem`
  // subclasses.
  enum { Type = UserType + 1 };
  int type() const;

  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

  virtual QRectF boundingRect() const;

  QSharedPointer<Page> page() const { return _page; }

  // Maps the point _point_ from the page's coordinate system (in pt) to this
  // item's coordinate system - chain with mapToScene and related methods to get
  // coordinates in other systems
  QPointF mapFromPage(const QPointF & point) const;
  // Maps the point _point_ from the item's coordinate system to the page's
  // coordinate system (in pt) - chain with mapFromScene and related methods to
  // convert from coordinates in other systems
  QPointF mapToPage(const QPointF & point) const;

  QTransform pageScale() { return _pageScale; }
  QTransform pointScale() { return _pointScale; }

protected:
  bool event(QEvent *event);

private:
  // Parent has no copy constructor.
  Q_DISABLE_COPY(PDFPageGraphicsItem)

private slots:
  void addLinks(QList< QSharedPointer<PDFLinkAnnotation> > links);

};

// FIXME: Should be turned into a QGraphicsPolygonItem
class PDFLinkGraphicsItem : public QGraphicsRectItem {
  typedef QGraphicsRectItem Super;

  QSharedPointer<PDFLinkAnnotation> _link;
  bool _activated;

public:
  PDFLinkGraphicsItem(QSharedPointer<PDFLinkAnnotation> a_link, QGraphicsItem *parent = 0);
  // See concerns in `PDFPageGraphicsItem` for why this feels fragile.
  enum { Type = UserType + 2 };
  int type() const;

protected:
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFLinkGraphicsItem)
};
// We need to declare a QList<PDFLinkGraphicsItem *> meta-type so we can
// pass it through inter-thread (i.e., queued) connections
Q_DECLARE_METATYPE(QList<PDFLinkGraphicsItem *>)


class PDFActionEvent : public QEvent {
  typedef QEvent Super;

public:
  PDFActionEvent(const PDFAction * action);
  static QEvent::Type ActionEvent;
  const PDFAction * action;
};

#endif // End header include guard

// vim: set sw=2 ts=2 et
