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
  PDFDocumentScene *pdf_scene;

  qreal zoomLevel;
  int _currentPage, _lastPage;

public:
  PDFDocumentView(QWidget *parent = 0);
  void setScene(PDFDocumentScene *a_scene);
  int currentPage();
  int lastPage();

public slots:
  void goPrev();
  void goNext();
  void goFirst();
  void goLast();
  void goToPage(int pageNum);

  void zoomIn();
  void zoomOut();

signals:
  void changedPage(int pageNum);
  void changedZoom(qreal zoomLevel);

protected:
  // Keep track of the current page by overloading the widget paint event.
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFDocumentView)
};


class PDFDocumentScene : public QGraphicsScene {
  Q_OBJECT
  typedef QGraphicsScene Super;

  const std::auto_ptr<Poppler::Document> doc;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> _pages;
  int _lastPage;

public:
  PDFDocumentScene(Poppler::Document *a_doc, QObject *parent = 0);
  QList<QGraphicsItem*> pages();
  QList<QGraphicsItem*> pages(const QPolygonF &polygon);
  int pageNumAt(const QPolygonF &polygon);

  int lastPage();
  // Poppler is *NOT* thread safe :(
  QMutex *docMutex;

signals:
  void pageChangeRequested(int pageNum);

protected:
  bool event(QEvent* event);

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

  Poppler::Page *page;
  QPixmap renderedPage;
  double dpiX;
  double dpiY;

  bool linksLoaded;
  QFutureWatcher< QList<PDFLinkGraphicsItem *> > *linkGenerator;

  bool pageIsRendering;
  QFutureWatcher<QImage> *pageImageGenerator;

  QTransform pageScale;
  qreal zoomLevel;

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
  QImage renderPage(qreal scaleFactor);

private slots:
  void addLinks();
  void updateRenderedPage();
};


class PDFLinkGraphicsItem : public QGraphicsRectItem {
  typedef QGraphicsRectItem Super;
  Poppler::Link *_link;

  bool activated;

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
