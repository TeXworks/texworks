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
#include <memory>
#include <QtGui/QtGui>
#include <poppler/qt4/poppler-qt4.h>

// Forward declare classes defined in this header.
class PDFDocumentScene;
class PDFPageGraphicsItem;
class PDFLinkGraphicsItem;

class PDFDocumentView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Super;

  // **TODO:** _Replace with an overloaded `scene` method._
  PDFDocumentScene *_pdf_scene;

  qreal _zoomLevel;
  int _currentPage, _lastPage;

public:
  // **TODO:** Implement SinglePage mode
  enum PageMode { PageMode_SinglePage, PageMode_OneColumnContinuous, PageMode_TwoColumnContinuous };

  PDFDocumentView(QWidget *parent = 0);
  void setScene(PDFDocumentScene *a_scene);
  int currentPage();
  int lastPage();
  PageMode pageMode() const { return _pageMode; }

public slots:
  void goPrev();
  void goNext();
  void goFirst();
  void goLast();
  void goToPage(int pageNum);
  void setPageMode(PageMode pageMode);
  void setSinglePageMode() { setPageMode(PageMode_SinglePage); }
  void setOneColContPageMode() { setPageMode(PageMode_OneColumnContinuous); }
  void setTwoColContPageMode() { setPageMode(PageMode_TwoColumnContinuous); }

  void zoomIn();
  void zoomOut();

signals:
  void changedPage(int pageNum);
  void changedZoom(qreal zoomLevel);

protected:
  // Keep track of the current page by overloading the widget paint event.
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);
  void moveTopLeftTo(const QPointF scenePos);

protected slots:
  void pageLayoutChanged();

private:
  PageMode _pageMode;
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFDocumentView)
};


// Class to render pages in the background
// Modelled after the "Blocking Fortune Client Example" in the Qt docs
// (http://doc.qt.nokia.com/stable/network-blockingfortuneclient.html)
class PDFPageRenderingThread : public QThread
{
  Q_OBJECT
public:
  PDFPageRenderingThread();
  virtual ~PDFPageRenderingThread();
  
  void requestRender(PDFPageGraphicsItem * page, qreal scaleFactor);

signals:
  void pageReady(PDFPageGraphicsItem *, qreal, QImage);

protected:
  virtual void run();

private:
  struct StackItem {
    PDFPageGraphicsItem * page;
    qreal scaleFactor;
  };
  QStack<StackItem> _workStack;
  QMutex _mutex;
  QWaitCondition _waitCondition;
  bool _quit;
};

// Cannot use QGraphicsGridLayout and similar classes for pages because it only
// works for QGraphicsLayoutItem (i.e., QGraphicsWidget)
class PDFPageGridLayout : public QObject {
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
  
public:
  PDFPageGridLayout();
  virtual ~PDFPageGridLayout() { }
  int columnCount() const { return _numCols; }
  int firstColumn() const { return _firstCol; }
  int xSpacing() const { return _xSpacing; }
  int ySpacing() const { return _ySpacing; }

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
};


class PDFDocumentScene : public QGraphicsScene {
  Q_OBJECT
  typedef QGraphicsScene Super;

  const std::auto_ptr<Poppler::Document> _doc;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> _pages;
  int _lastPage;
  PDFPageRenderingThread _renderingThread;
  PDFPageGridLayout _pageLayout;

public:
  PDFDocumentScene(Poppler::Document *a_doc, QObject *parent = 0);
  QList<QGraphicsItem*> pages();
  QList<QGraphicsItem*> pages(const QPolygonF &polygon);
  int pageNumAt(const QPolygonF &polygon);
  PDFPageRenderingThread& renderingThread() { return _renderingThread; }
  PDFPageGridLayout& pageLayout() { return _pageLayout; }

  int lastPage();
  // Poppler is *NOT* thread safe :(
  QMutex *docMutex;

signals:
  void pageChangeRequested(int pageNum);
  void pageLayoutChanged();

protected:
  bool event(QEvent* event);

protected slots:
  void pageLayoutChanged(const QRectF& sceneRect);

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFDocumentScene)
};


// Also inherits QObject in order to access SIGNALS/SLOTS for `QFutureWatcher`.
// A little hokey. Should probably inherit `QGraphicsObject` and be a
// completely custom implementation.
class PDFPageGraphicsItem : public QObject, public QGraphicsPixmapItem
{
  Q_OBJECT
  typedef QGraphicsPixmapItem Super;

  Poppler::Page *_page;
  QPixmap _renderedPage;
  double _dpiX;
  double _dpiY;

  bool _linksLoaded;
  QFutureWatcher< QList<PDFLinkGraphicsItem *> > *_linkGenerator;

  bool _pageIsRendering;

  QTransform _pageScale;
  qreal _zoomLevel;
  PDFPageRenderingThread * _connectedRenderingThread;
  
  friend class PDFPageRenderingThread;
  friend class PDFPageGridLayout;

public:

  PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent = 0);

  // This seems fragile as it assumes no other code declaring a custom graphics
  // item will choose the same ID for it's object types. Unfortunately, there
  // appears to be no equivalent of `registerEventType` for `QGraphicsItem`
  // subclasses.
  enum { Type = UserType + 1 };
  int type() const;

  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
  // Parent has no copy constructor.
  Q_DISABLE_COPY(PDFPageGraphicsItem)

  QList<PDFLinkGraphicsItem *> loadLinks();

private slots:
  void addLinks();
  void maybeUpdateRenderedPage(PDFPageGraphicsItem * page, qreal scaleFactor, QImage pageImage);
};


class PDFLinkGraphicsItem : public QGraphicsRectItem {
  typedef QGraphicsRectItem Super;

  Poppler::Link *_link;
  bool _activated;

public:
  PDFLinkGraphicsItem(Poppler::Link *a_link, QGraphicsItem *parent = 0);
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


class PDFLinkEvent : public QEvent {
  typedef QEvent Super;

public:
  PDFLinkEvent(int a_page);
  static QEvent::Type LinkEvent;
  const int pageNum;
};

// vim: set sw=2 ts=2 et

