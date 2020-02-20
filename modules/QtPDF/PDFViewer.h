/**
 * Copyright (C) 2013-2020  Charlie Sharpsteen, Stefan Löffler
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
#include <QtGui/QtGui>
#include "PDFDocumentWidget.h"

class PageCounter;
class ZoomTracker;
class SearchLineEdit;

class PDFViewer : public QMainWindow {
  Q_OBJECT

  PageCounter * _counter;
  ZoomTracker * _zoomWdgt;
  SearchLineEdit * _search;
  QToolBar * _toolBar;

public:
  PDFViewer(const QString & pdf_doc = QString(), QWidget * parent = Q_NULLPTR, Qt::WindowFlags flags = Q_NULLPTR);

public slots:
  void open();

private slots:
  void openUrl(const QUrl url) const;
  void openPdf(QString filename, QtPDF::PDFDestination destination, bool newWindow) const;
  void syncFromPdf(const int page, const QPointF pos);
  void searchProgressChanged(int percent, int occurrences);
  void documentChanged(const QWeakPointer<QtPDF::Backend::Document> newDoc);
  
#ifdef DEBUG
  // FIXME: Remove this
  void setGermanLocale() { emit switchInterfaceLocale(QLocale(QLocale::German)); }
  void setEnglishLocale() { emit switchInterfaceLocale(QLocale(QLocale::C)); }
#endif

signals:
  void switchInterfaceLocale(const QLocale & newLocale);
};


class SearchLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  SearchLineEdit(QWidget * parent = Q_NULLPTR);

protected:
  void resizeEvent(QResizeEvent *) override;

private:
  QToolButton *nextResultButton, *previousResultButton, *clearButton;

signals:
  void searchRequested(QString searchText);
  void gotoNextResult();
  void gotoPreviousResult();
  void searchCleared();

private slots:
  void prepareSearch();
  void clearSearch();
  void handleNextResult();
  void handlePreviousResult();

};


class PageCounter : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  int currentPage{1}, lastPage{-1};

public:
  PageCounter(QWidget * parent = Q_NULLPTR, Qt::WindowFlags f = {});

public slots:
  void setLastPage(int page);
  void setCurrentPage(int page);

private:
  void refreshText();
};


class ZoomTracker : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  qreal zoom{1.0};

public:
  ZoomTracker(QWidget * parent = Q_NULLPTR, Qt::WindowFlags f = {});

public slots:
  void setZoom(qreal newZoom);

private:
  void refreshText();
};
