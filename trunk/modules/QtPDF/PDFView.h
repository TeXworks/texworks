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
  bool dirty;
  double dpiX;
  double dpiY;

public:

  PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent = 0);
  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFPageGraphicsItem)
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
  void goToPage(int pageNum);

public slots:
  void zoomIn();
  void zoomOut();

signals:
  void changedPage(int);
  void changedZoom(qreal);

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


class PageCounter : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  int currentPage, lastPage;

public:
  PageCounter(QWidget *parent = 0, Qt::WindowFlags f = 0);

public slots:
  void setLastPage(int page);
  void setCurrentPage(int page);

private:
  void refreshText();
};


class ZoomTracker : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  qreal zoom;

public:
  ZoomTracker(QWidget *parent = 0, Qt::WindowFlags f = 0);

public slots:
  void setZoom(qreal newZoom);

private:
  void refreshText();
};
