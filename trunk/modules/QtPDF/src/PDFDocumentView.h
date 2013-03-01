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
#include <PDFDocumentTools.h>

namespace QtPDF {

// Forward declare classes defined in this header.
class PDFDocumentScene;
class PDFPageGraphicsItem;
class PDFLinkGraphicsItem;
class PDFDocumentMagnifierView;
class PDFActionEvent;
class PDFDocumentView;


const int TILE_SIZE=1024;

class PDFDocumentView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView Super;

  QSharedPointer<PDFDocumentScene> _pdf_scene;

  qreal _zoomLevel;
  int _currentPage, _lastPage;

  QString _searchString;
  QList<QGraphicsItem *> _searchResults;
  QFutureWatcher< QList<SearchResult> > _searchResultWatcher;
  int _currentSearchResult;
  bool _useGrayScale;

  friend class DocumentTool::AbstractTool;

public:
  enum PageMode { PageMode_SinglePage, PageMode_OneColumnContinuous, PageMode_TwoColumnContinuous };
  enum MouseMode { MouseMode_MagnifyingGlass, MouseMode_Move, MouseMode_MarqueeZoom };
  enum Dock { Dock_TableOfContents, Dock_MetaData, Dock_Fonts, Dock_Permissions, Dock_Annotations };

  PDFDocumentView(QWidget *parent = 0);
  ~PDFDocumentView();
  void setScene(QSharedPointer<PDFDocumentScene> a_scene);
  int currentPage();
  int lastPage();
  PageMode pageMode() const { return _pageMode; }
  qreal zoomLevel() const { return _zoomLevel; }
  bool useGrayScale() const { return _useGrayScale; }

  // The ownership of the returned pointers is transferred to the caller (i.e.,
  // he has to destroy them, unless the `parent` widget does that automatically)
  // They are fully wired to this PDFDocumentView (e.g., clicking on entries in
  // the table of contents will change this view)
  QDockWidget * dockWidget(const Dock type, QWidget * parent = NULL);
  
  DocumentTool::AbstractTool * armedTool() const { return _armedTool; }
  void triggerContextClick(const int page, const QPointF pos) { emit contextClick(page, pos); }

public slots:
  void goPrev();
  void goNext();
  void goFirst();
  void goLast();
  // `alignment` can be (a combination of) 0, Qt::AlignLeft, Qt::AlignRight,
  // Qt::AlignHCenter, Qt::AlignTop, Qt::AlignBottom, Qt::AlignVCenter.
  // 0 corresponds to no alignment, i.e., the view will change so that the
  // rectangle of page pageNum closest to the original viewport rect is visible.
  void goToPage(const int pageNum, const int alignment = Qt::AlignLeft | Qt::AlignTop);
  // Similar to the one above, but view is aligned at `anchor`. Note that the
  // default alignment is centering here, which is also used if `alignment` == 0.
  // `anchor` must be given in item coordinates
  void goToPage(const int pageNum, const QPointF anchor, const int alignment = Qt::AlignHCenter | Qt::AlignVCenter);
  void setPageMode(PageMode pageMode);
  void setSinglePageMode() { setPageMode(PageMode_SinglePage); }
  void setOneColContPageMode() { setPageMode(PageMode_OneColumnContinuous); }
  void setTwoColContPageMode() { setPageMode(PageMode_TwoColumnContinuous); }
  void setMouseMode(const MouseMode newMode);
  void setMouseModeMagnifyingGlass() { setMouseMode(MouseMode_MagnifyingGlass); }
  void setMouseModeMove() { setMouseMode(MouseMode_Move); }
  void setMouseModeMarqueeZoom() { setMouseMode(MouseMode_MarqueeZoom); }
  void setMagnifierShape(const DocumentTool::MagnifyingGlass::MagnifierShape shape);
  void setMagnifierSize(const int size);
  void setUseGrayScale(const bool grayScale = true) { _useGrayScale = grayScale; }

  void zoomBy(qreal zoomFactor);
  void zoomIn();
  void zoomOut();
  void zoomToRect(QRectF a_rect);
  void zoomFitWindow();
  void zoomFitWidth();
  void zoom100();

  void search(QString searchText);
  void nextSearchResult();
  void previousSearchResult();
  void clearSearchResults();

signals:
  void changedPage(int pageNum);
  void changedZoom(qreal zoomLevel);
  // emitted, e.g., if a new document was loaded, or if the existing document
  // has changed (e.g., if it was unlocked)
  void changedDocument(const QSharedPointer<QtPDF::Document> newDoc);

  void searchProgressChanged(int percent, int occurrences);

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
  void changeEvent(QEvent * event);
  
  // Maybe this will become public later on
  // Ownership of tool is transferred to PDFDocumentView
  void registerTool(DocumentTool::AbstractTool * tool);

  DocumentTool::AbstractTool * getToolByType(const DocumentTool::AbstractTool::Type type);

  void armTool(const DocumentTool::AbstractTool::Type toolType);
  void armTool(DocumentTool::AbstractTool * tool);
  void disarmTool();

protected slots:
  void maybeUpdateSceneRect();
  void maybeArmTool(uint modifiers);
  void pdfActionTriggered(const QtPDF::PDFAction * action);
  // Note: view specifies which part of the page should be visible and must
  // therefore be given in page coordinates
  void goToPage(const PDFPageGraphicsItem * page, const QRectF view, const bool mayZoom = false);
  void goToPage(const PDFPageGraphicsItem * page, const int alignment = Qt::AlignLeft | Qt::AlignTop);
  void goToPage(const PDFPageGraphicsItem * page, const QPointF anchor, const int alignment = Qt::AlignHCenter | Qt::AlignVCenter);
  void searchResultReady(int index);
  void searchProgressValueChanged(int progressValue);
  void switchInterfaceLocale(const QLocale & newLocale);

private:
  PageMode _pageMode;
  MouseMode _mouseMode;
  QCursor _hiddenCursor;
  QVector<DocumentTool::AbstractTool*> _tools;
  DocumentTool::AbstractTool * _armedTool;
  QMap<uint, DocumentTool::AbstractTool*> _toolAccessors;
  
  static QTranslator * _translator;
  static QString _translatorLanguage;

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

  DocumentTool::MagnifyingGlass::MagnifierShape _shape;
  int _size;

public:
  PDFDocumentMagnifierView(PDFDocumentView *parent = 0);
  // the zoom factor multiplies the parent view's _zoomLevel
  void setZoomFactor(const qreal zoomFactor);
  void setPosition(const QPoint pos);
  void setShape(const DocumentTool::MagnifyingGlass::MagnifierShape shape);
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


class PDFDocumentInfoWidget : public QWidget
{
  Q_OBJECT
  friend class PDFDocumentView;
public:
  PDFDocumentInfoWidget(QWidget * parent = NULL, const QString & title = QString()) : QWidget(parent) { setWindowTitle(title); }
  virtual ~PDFDocumentInfoWidget() { }
  // If the widget has a fixed size, it should not be resized (it can, e.g., be
  // put into a QScrollArea instead).
public slots:
  void setWindowTitle(const QString & windowTitle);
signals:
  void windowTitleChanged(const QString &);
protected slots:
  virtual void initFromDocument(const QSharedPointer<QtPDF::Document> doc) { _doc = doc; }
  virtual void retranslateUi() { };
  virtual void clear() = 0;
protected:
  virtual void changeEvent(QEvent * event);
  // we need to keep a reference to the document to allow dynamic lookup of data
  // (e.g., when retranslating the widget)
  QSharedPointer<QtPDF::Document> _doc;
};

class PDFToCInfoWidget : public PDFDocumentInfoWidget
{
  Q_OBJECT
public:
  PDFToCInfoWidget(QWidget * parent);
  virtual ~PDFToCInfoWidget();

protected slots:
  void initFromDocument(const QSharedPointer<QtPDF::Document> doc);
  void clear();
  virtual void retranslateUi();
signals:
  void actionTriggered(const QtPDF::PDFAction*);
private slots:
  void itemSelectionChanged();
private:
  static void recursiveAddTreeItems(const QList<PDFToCItem> & tocItems, QTreeWidgetItem * parentTreeItem);
  static void recursiveClearTreeItems(QTreeWidgetItem * parent);
  QTreeWidget * _tree;
};

class PDFMetaDataInfoWidget : public PDFDocumentInfoWidget
{
  Q_OBJECT
public:
  PDFMetaDataInfoWidget(QWidget * parent);
  virtual ~PDFMetaDataInfoWidget() { }
  
protected slots:
  void initFromDocument(const QSharedPointer<QtPDF::Document> doc);
  void clear();
  virtual void retranslateUi();
  void reload();
private:
  QGroupBox * _documentGroup;
  QLabel * _title, * _titleLabel;
  QLabel * _author, * _authorLabel;
  QLabel * _subject, * _subjectLabel;
  QLabel * _keywords, * _keywordsLabel;
  QGroupBox * _processingGroup;
  QLabel * _creator, * _creatorLabel;
  QLabel * _producer, * _producerLabel;
  QLabel * _creationDate, * _creationDateLabel;
  QLabel * _modDate, * _modDateLabel;
  QLabel * _trapped, * _trappedLabel;
  QGroupBox * _otherGroup;
};

class PDFFontsInfoWidget : public PDFDocumentInfoWidget
{
  Q_OBJECT
public:
  PDFFontsInfoWidget(QWidget * parent);
  virtual ~PDFFontsInfoWidget() { }
  
protected slots:
  void initFromDocument(const QSharedPointer<QtPDF::Document> doc);
  void clear();
  virtual void retranslateUi();
  void reload();
private:
  QTableWidget * _table;
};

class PDFPermissionsInfoWidget : public PDFDocumentInfoWidget
{
  Q_OBJECT
public:
  PDFPermissionsInfoWidget(QWidget * parent);
  virtual ~PDFPermissionsInfoWidget() { }
  
protected slots:
  void initFromDocument(const QSharedPointer<QtPDF::Document> doc);
  void clear();
  virtual void retranslateUi();
  void reload();
private:
  QLabel * _print, * _printLabel;
  QLabel * _modify, * _modifyLabel;
  QLabel * _extract, * _extractLabel;
  QLabel * _addNotes, * _addNotesLabel;
  QLabel * _form, * _formLabel;
};

class PDFAnnotationsInfoWidget : public PDFDocumentInfoWidget
{
  Q_OBJECT

  QFutureWatcher< QList< QSharedPointer<PDFAnnotation> > > _annotWatcher;
  QTableWidget * _table;

  static QList< QSharedPointer<PDFAnnotation> > loadAnnotations(QSharedPointer<Page> page);

public:
  PDFAnnotationsInfoWidget(QWidget * parent);
  virtual ~PDFAnnotationsInfoWidget() { }
    
protected slots:
  void initFromDocument(const QSharedPointer<QtPDF::Document> doc);
  void clear();
  virtual void retranslateUi();
  void annotationsReady(int index);
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
  int pageNumFor(const PDFPageGraphicsItem * const graphicsItem) const;
  PDFPageLayout& pageLayout() { return _pageLayout; }

  void showOnePage(const int pageIdx) const;
  void showOnePage(const PDFPageGraphicsItem * page) const;
  void showAllPages() const;

  int lastPage();

  const QSharedPointer<Document> document() const { return _doc; }

signals:
  void pageChangeRequested(int pageNum);
  void pageLayoutChanged();
  void pdfActionTriggered(const QtPDF::PDFAction * action);
  void documentChanged(const QSharedPointer<QtPDF::Document> doc);

public slots:
  void doUnlockDialog();
  void retranslateUi();

protected slots:
  void pageLayoutChanged(const QRectF& sceneRect);
  void reinitializeScene();

protected:
  bool event(QEvent* event);
  
  QWidget * _unlockWidget;
  QLabel * _unlockWidgetLockText, * _unlockWidgetLockIcon;
  QPushButton * _unlockWidgetUnlockButton;

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
  bool _annotationsLoaded;

  QTransform _pageScale, _pointScale;
  qreal _zoomLevel;

  friend class PageProcessingRenderPageRequest;
  friend class PageProcessingLoadLinksRequest;
  friend class PDFPageLayout;

  static void imageToGrayScale(QImage & img);

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
  void addAnnotations(QList< QSharedPointer<PDFAnnotation> > annotations);
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
  void retranslateUi();

protected:
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFLinkGraphicsItem)
};


// FIXME: Should be turned into a QGraphicsPolygonItem
class PDFMarkupAnnotationGraphicsItem : public QGraphicsRectItem {
  typedef QGraphicsRectItem Super;

  QSharedPointer<PDFMarkupAnnotation> _annot;
  bool _activated;
  QWidget * _popup;

public:
  PDFMarkupAnnotationGraphicsItem(QSharedPointer<PDFMarkupAnnotation> annot, QGraphicsItem *parent = 0);
  // See concerns in `PDFPageGraphicsItem` for why this feels fragile.
  enum { Type = UserType + 3 };
  int type() const;

protected:
  void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

  void mousePressEvent(QGraphicsSceneMouseEvent *event);
  void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFMarkupAnnotationGraphicsItem)
};

class PDFActionEvent : public QEvent {
  typedef QEvent Super;

public:
  PDFActionEvent(const PDFAction * action);
  static QEvent::Type ActionEvent;
  const PDFAction * action;
};

} // namespace QtPDF

// Note: Q_DECLARE_METATYPE must be specified _outside_ any namespace
// declaration (according to Qt docs)

// We need to declare a QList<PDFLinkGraphicsItem *> meta-type so we can
// pass it through inter-thread (i.e., queued) connections
Q_DECLARE_METATYPE(QList<QtPDF::PDFLinkGraphicsItem *>)
// We need to declare a QList<PDFMarkupAnnotationGraphicsItem *> meta-type so we can
// pass it through inter-thread (i.e., queued) connections
Q_DECLARE_METATYPE(QList<QtPDF::PDFMarkupAnnotationGraphicsItem *>)


#endif // End header include guard

// vim: set sw=2 ts=2 et
