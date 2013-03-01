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


class PDFPageGraphicsItem : public QGraphicsPixmapItem {
  typedef QGraphicsPixmapItem super;
  // To spare the need for a destructor
  const std::auto_ptr<Poppler::Page> page;
  QPixmap renderedPage;
  double dpiX;
  double dpiY;

  bool dirty;
  qreal zoomLevel;

public:

  PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent = 0);
  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFPageGraphicsItem)
};


class PDFLinkGraphicsItem : public QGraphicsRectItem {
  typedef QGraphicsRectItem super;
  Poppler::Link *_link;

  bool activated;

public:
  PDFLinkGraphicsItem(Poppler::Link *a_link, QGraphicsItem *parent = 0);

protected:
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFLinkGraphicsItem)
};


class PDFDocumentView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView super;
  const std::auto_ptr<Poppler::Document> doc;
  qreal zoomLevel;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> pages;
  int _currentPage, _lastPage;

public:
  PDFDocumentView(Poppler::Document *a_doc, QWidget *parent = 0);
  ~PDFDocumentView();
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

  // **TODO:**
  // _This class basically comes with a built-in `QGraphicsScene` unlike a
  // traditional `QGraphicsView` where the scenes can be swapped around. Should
  // we disable the `setScene` function by declaring it `private`? Does it make
  // sense to have different graphics scenes?_
};
