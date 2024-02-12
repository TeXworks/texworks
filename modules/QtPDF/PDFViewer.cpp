/**
 * Copyright (C) 2013-2023  Charlie Sharpsteen, Stefan Löffler
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
#include "PDFViewer.h"

QTranslator * PDFViewer::_translator = nullptr;
QString PDFViewer::_translatorLanguage;

PDFViewer::PDFViewer(const QString & pdf_doc, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags)
{
  QtPDF::PDFDocumentWidget *docWidget = new QtPDF::PDFDocumentWidget(this);
  connect(this, SIGNAL(switchInterfaceLocale(QLocale)), docWidget, SLOT(switchInterfaceLocale(QLocale)));

  if (!pdf_doc.isEmpty())
    docWidget->load(pdf_doc);
  docWidget->goFirst();

  _counter = new PageCounter(this->statusBar());
  _zoomWdgt = new ZoomTracker(this);
  _search = new SearchLineEdit(this);
  _toolBar = new QToolBar(this);

  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("document-open")), tr("Open..."), this, SLOT(open()));
  _toolBar->addSeparator();

  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("zoom-in")), tr("Zoom In"), docWidget, SLOT(zoomIn()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("zoom-out")), tr("Zoom Out"), docWidget, SLOT(zoomOut()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("zoom-fit-width")), tr("Fit to Width"), docWidget, SLOT(zoomFitWidth()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("zoom-fit-best")), tr("Fit to Window"), docWidget, SLOT(zoomFitWindow()));

  _toolBar->addSeparator();
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("view-pages-single")), tr("Single Page Mode"), docWidget, SLOT(setSinglePageMode()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("view-pages-continuous")), tr("One Column Continuous Page Mode"), docWidget, SLOT(setOneColContPageMode()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("view-pages-facing-continuous")), tr("Two Columns Continuous Page Mode"), docWidget, SLOT(setTwoColContPageMode()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("view-presentation")), tr("Presentation Mode"), docWidget, SLOT(setPresentationMode()));
  // TODO: fullscreen mode for presentations

  _toolBar->addSeparator();
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("tool-magnifier")), tr("Magnify"), docWidget, SLOT(setMouseModeMagnifyingGlass()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("tool-pan")), tr("Pan"), docWidget, SLOT(setMouseModeMove()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("tool-zoom-select")), tr("Marquee Zoom"), docWidget, SLOT(setMouseModeMarqueeZoom()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("tool-measure")), tr("Measure"), docWidget, SLOT(setMouseModeMeasure()));
  _toolBar->addAction(QIcon::fromTheme(QStringLiteral("tool-select-text")), tr("Select"), docWidget, SLOT(setMouseModeSelect()));

  _counter->setLastPage(docWidget->lastPage());
  connect(docWidget, SIGNAL(changedPage(int)), _counter, SLOT(setCurrentPage(int)));
  connect(docWidget, SIGNAL(changedZoom(qreal)), _zoomWdgt, SLOT(setZoom(qreal)));
  connect(docWidget, SIGNAL(requestOpenUrl(const QUrl)), this, SLOT(openUrl(const QUrl)));
  connect(docWidget, SIGNAL(requestOpenPdf(QString, QtPDF::PDFDestination, bool)), this, SLOT(openPdf(QString, QtPDF::PDFDestination, bool)));
  connect(docWidget, SIGNAL(contextClick(const int, const QPointF)), this, SLOT(syncFromPdf(const int, const QPointF)));
  connect(docWidget, SIGNAL(searchProgressChanged(int, int)), this, SLOT(searchProgressChanged(int, int)));
  connect(docWidget, SIGNAL(changedDocument(const QWeakPointer<QtPDF::Backend::Document>)), this, SLOT(documentChanged(const QWeakPointer<QtPDF::Backend::Document>)));

  _toolBar->addSeparator();
#ifdef DEBUG
  // TODO: Make this more general or remove it altogether
  _toolBar->addAction(QString::fromUtf8("en"), this, SLOT(setEnglishLocale()));
  _toolBar->addAction(QString::fromUtf8("de"), this, SLOT(setGermanLocale()));
  _toolBar->addSeparator();
#endif
  _toolBar->addWidget(_search);
  connect(_search, SIGNAL(searchRequested(QString)), docWidget, SLOT(search(QString)));
  connect(_search, SIGNAL(gotoNextResult()), docWidget, SLOT(nextSearchResult()));
  connect(_search, SIGNAL(gotoPreviousResult()), docWidget, SLOT(previousSearchResult()));
  connect(_search, SIGNAL(searchCleared()), docWidget, SLOT(clearSearchResults()));

  statusBar()->addPermanentWidget(_counter);
  statusBar()->addWidget(_zoomWdgt);
  addToolBar(_toolBar);
  setCentralWidget(docWidget);

  QDockWidget * toc = docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_TableOfContents, this);
  addDockWidget(Qt::LeftDockWidgetArea, toc);
  tabifyDockWidget(toc, docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_MetaData, this));
  tabifyDockWidget(toc, docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_Fonts, this));
  tabifyDockWidget(toc, docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_Permissions, this));
  tabifyDockWidget(toc, docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_Annotations, this));
  tabifyDockWidget(toc, docWidget->dockWidget(QtPDF::PDFDocumentView::Dock_OptionalContent, this));
  toc->raise();

  QShortcut * goPrevViewRect = new QShortcut(QKeySequence(tr("Alt+Left")), this);
  connect(goPrevViewRect, SIGNAL(activated()), docWidget, SLOT(goPrevViewRect()));
}

void PDFViewer::open()
{
  QString pdf_doc = QFileDialog::getOpenFileName(this, tr("Open PDF Document"), QString(), tr("PDF documents (*.pdf)"));
  if (pdf_doc.isEmpty())
    return;

  QtPDF::PDFDocumentWidget * docWidget = qobject_cast<QtPDF::PDFDocumentWidget*>(centralWidget());
  if (!docWidget) {
    return;
  }
  docWidget->load(pdf_doc);
}

void PDFViewer::documentChanged(const QWeakPointer<QtPDF::Backend::Document> newDoc)
{
  if (_counter) {
    QSharedPointer<QtPDF::Backend::Document> doc(newDoc.toStrongRef());
    _counter->setLastPage(doc->numPages());
  }
}

void PDFViewer::searchProgressChanged(int percent, int occurrences)
{
  statusBar()->showMessage(tr("%1% of the document searched (%2 occurrences found)").arg(percent).arg(occurrences));
}

void PDFViewer::openUrl(const QUrl url) const
{
  // **FIXME:** _We don't handle this yet!_
  if( url.scheme() == QString::fromUtf8("file") )
    return;

  // **TODO:**
  // `openUrl` can fail and that needs to be handled._
  QDesktopServices::openUrl(url);
}

void PDFViewer::openPdf(QString filename, QtPDF::PDFDestination destination, bool newWindow) const
{
  qDebug() << "Open PDF" << filename << "on page" << (destination.page() + 1) << "/ at anchor" << destination.destinationName() << "(in new window =" << newWindow << ")";
}

void PDFViewer::syncFromPdf(const int page, const QPointF pos)
{
  qDebug() << "Invoke SyncTeX from page" << (page + 1) << "at" << pos;
}

void PDFViewer::switchInterfaceLocale(const QLocale & newLocale)
{
  // TODO: Allow for a custom directory for .qm files (i.e., one in the
  // filesystem, instead of the embedded resources)
  if (_translatorLanguage == newLocale.name())
    return;

  // Remove the old translator (if any)
  if (_translator) {
    QCoreApplication::removeTranslator(_translator);
    _translator->deleteLater();
    _translator = nullptr;
  }

  _translatorLanguage = newLocale.name();

  _translator = new QTranslator();
  if (_translator->load(QString::fromUtf8("QtPDF_%1").arg(newLocale.name()), QString::fromUtf8(":/resfiles/translations")))
    QCoreApplication::installTranslator(_translator);
  else {
    _translator->deleteLater();
    _translator = nullptr;
  }
}


PageCounter::PageCounter(QWidget *parent, Qt::WindowFlags f) : super(parent, f)
{
  refreshText();
}

void PageCounter::setLastPage(int page){
  lastPage = page;
  refreshText();
}

void PageCounter::setCurrentPage(int page){
  // Assume the `SIGNAL` attached to this slot is emitting page numbers as
  // indicies in a 0-based array.
  currentPage = page + 1;
  refreshText();
}

void PageCounter::refreshText() {
  if (lastPage > 0 && currentPage > 0 && currentPage <= lastPage)
    setText(tr("Page %1 of %2").arg(currentPage).arg(lastPage));
  else
    setText(QString::fromLatin1(""));
  update();
}


ZoomTracker::ZoomTracker(QWidget *parent, Qt::WindowFlags f) : super(parent, f)
{
  refreshText();
}

void ZoomTracker::setZoom(qreal newZoom){
  zoom = newZoom;
  refreshText();
}

void ZoomTracker::refreshText() {
  setText(tr("Zoom %1%").arg(zoom * 100));
  update();
}

// The SearchLineEdit class is adapted from code presented by Girish
// Ramakrishnan in a Qt Labs post:
//
//   http://labs.qt.nokia.com/2007/06/06/lineedit-with-a-clear-button
SearchLineEdit::SearchLineEdit(QWidget *parent):
  QLineEdit(parent)
{
  previousResultButton = new QToolButton(this);
  previousResultButton->setIcon(style()->standardIcon(QStyle::SP_ArrowLeft));
  previousResultButton->setCursor(Qt::ArrowCursor);
  previousResultButton->setStyleSheet(QString::fromUtf8("QToolButton { border: none; padding: 0px; }"));
  connect(previousResultButton, SIGNAL(clicked()), this, SLOT(handlePreviousResult()));

  nextResultButton = new QToolButton(this);
  nextResultButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
  nextResultButton->setCursor(Qt::ArrowCursor);
  nextResultButton->setStyleSheet(QString::fromUtf8("QToolButton { border: none; padding: 0px; }"));
  connect(nextResultButton, SIGNAL(clicked()), this, SLOT(handleNextResult()));

  clearButton = new QToolButton(this);
  clearButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet(QString::fromUtf8("QToolButton { border: none; padding: 0px; }"));
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clearSearch()));

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString::fromUtf8("QLineEdit { padding-right: %1px; } ").arg(
      nextResultButton->sizeHint().width() +
      previousResultButton->sizeHint().width() +
      clearButton->sizeHint().width() + frameWidth + 1));
  QSize msz = minimumSizeHint();
  setMinimumSize(qMax(msz.width(), nextResultButton->sizeHint().width() +
      previousResultButton->sizeHint().width() +
      clearButton->sizeHint().width() + frameWidth * 2 + 2),
      qMax(msz.height(), clearButton->sizeHint().height() + frameWidth * 2 + 2));

  connect(this, SIGNAL(returnPressed()), this, SLOT(prepareSearch()));

  setPlaceholderText(QString::fromUtf8("Search"));
}

void SearchLineEdit::resizeEvent(QResizeEvent *)
{
  QSize sa = previousResultButton->sizeHint(),
        sb = nextResultButton->sizeHint(),
        sc = clearButton->sizeHint();

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);

  previousResultButton->move(rect().right() - frameWidth - sa.width() - sb.width() - sc.width(),
                    (rect().bottom() + 1 - sa.height())/2);
  nextResultButton->move(rect().right() - frameWidth - sb.width() - sc.width(),
                    (rect().bottom() + 1 - sb.height())/2);
  clearButton->move(rect().right() - frameWidth - sc.width(),
                    (rect().bottom() + 1 - sc.height())/2);
}

void SearchLineEdit::prepareSearch() {
  if( this->text().isEmpty() )
    return;

  emit searchRequested(this->text());
}

void SearchLineEdit::clearSearch() {
  // Don't check for empty text as the user may have deleted the text, then hit
  // the clear button. In this case, there are still other objects that may
  // want to recieve the `searchCleared` signal.
  clear();

  emit searchCleared();
}

void SearchLineEdit::handleNextResult() {
  if( this->text().isEmpty() )
    return;

  emit gotoNextResult();
}

void SearchLineEdit::handlePreviousResult() {
  if( this->text().isEmpty() )
    return;

  emit gotoPreviousResult();
}


// vim: set sw=2 ts=2 et
