#include "PDFViewer.h"

PDFViewer::PDFViewer(const QString pdf_doc, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags)
{
#ifdef USE_MUPDF
  QtPDF::Backend::Document *a_pdf_doc = new QtPDF::Backend::MuPDF::Document(pdf_doc);
#elif USE_POPPLER
  QtPDF::Backend::Document *a_pdf_doc = new QtPDF::Backend::Poppler::Document(pdf_doc);
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

  QtPDF::PDFDocumentView *docView = new QtPDF::PDFDocumentView(this);
  connect(this, SIGNAL(switchInterfaceLocale(QLocale)), docView, SLOT(switchInterfaceLocale(QLocale)));

  if (a_pdf_doc) {
    // Note: Don't pass `this` (or any other QObject*) as parent to the new
    // PDFDocumentScene as that would cause docScene to be destroyed with its
    // parent, thereby bypassing the QSharedPointer mechanism. docScene will be
    // freed automagically when the last QSharedPointer pointing to it will be
    // destroyed.
    QSharedPointer<QtPDF::PDFDocumentScene> docScene(new QtPDF::PDFDocumentScene(a_pdf_doc));
    docView->setScene(docScene);
  }
  docView->goFirst();

  _counter = new PageCounter(this->statusBar());
  _zoomWdgt = new ZoomTracker(this);
  _search = new SearchLineEdit(this);
  _toolBar = new QToolBar(this);

  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/document-open.png")), tr("Open..."), this, SLOT(open()));
  _toolBar->addSeparator();

  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoomin.png")), tr("Zoom In"), docView, SLOT(zoomIn()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoomout.png")), tr("Zoom Out"), docView, SLOT(zoomOut()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/fitwidth.png")), tr("Fit to Width"), docView, SLOT(zoomFitWidth()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/fitwindow.png")), tr("Fit to Window"), docView, SLOT(zoomFitWindow()));

  _toolBar->addSeparator();
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/pagemode-single.png")), tr("Single Page Mode"), docView, SLOT(setSinglePageMode()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/pagemode-continuous.png")), tr("One Column Continuous Page Mode"), docView, SLOT(setOneColContPageMode()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/pagemode-twocols.png")), tr("Two Columns Continuous Page Mode"), docView, SLOT(setTwoColContPageMode()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/pagemode-present.png")), tr("Presentation Mode"), docView, SLOT(setPresentationMode()));
  // TODO: fullscreen mode for presentations

  _toolBar->addSeparator();
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoom.png")), tr("Magnify"), docView, SLOT(setMouseModeMagnifyingGlass()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/hand.png")), tr("Pan"), docView, SLOT(setMouseModeMove()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoom-select.png")), tr("Marquee Zoom"), docView, SLOT(setMouseModeMarqueeZoom()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/measure.png")), tr("Measure"), docView, SLOT(setMouseModeMeasure()));
  _toolBar->addAction(QIcon(QString::fromUtf8(":/icons/select-text.png")), tr("Select"), docView, SLOT(setMouseModeSelect()));

  _counter->setLastPage(docView->lastPage());
  connect(docView, SIGNAL(changedPage(int)), _counter, SLOT(setCurrentPage(int)));
  connect(docView, SIGNAL(changedZoom(qreal)), _zoomWdgt, SLOT(setZoom(qreal)));
  connect(docView, SIGNAL(requestOpenUrl(const QUrl)), this, SLOT(openUrl(const QUrl)));
  connect(docView, SIGNAL(requestOpenPdf(QString, int, bool)), this, SLOT(openPdf(QString, int, bool)));
  connect(docView, SIGNAL(contextClick(const int, const QPointF)), this, SLOT(syncFromPdf(const int, const QPointF)));
  connect(docView, SIGNAL(searchProgressChanged(int, int)), this, SLOT(searchProgressChanged(int, int)));
  connect(docView, SIGNAL(changedDocument(const QSharedPointer<QtPDF::Backend::Document>)), this, SLOT(documentChanged(const QSharedPointer<QtPDF::Backend::Document>)));

  _toolBar->addSeparator();
#ifdef DEBUG
  // FIXME: Remove this
  _toolBar->addAction(QString::fromUtf8("en"), this, SLOT(setEnglishLocale()));
  _toolBar->addAction(QString::fromUtf8("de"), this, SLOT(setGermanLocale()));
  _toolBar->addSeparator();
#endif
  _toolBar->addWidget(_search);
  connect(_search, SIGNAL(searchRequested(QString)), docView, SLOT(search(QString)));
  connect(_search, SIGNAL(gotoNextResult()), docView, SLOT(nextSearchResult()));
  connect(_search, SIGNAL(gotoPreviousResult()), docView, SLOT(previousSearchResult()));
  connect(_search, SIGNAL(searchCleared()), docView, SLOT(clearSearchResults()));

  statusBar()->addPermanentWidget(_counter);
  statusBar()->addWidget(_zoomWdgt);
  addToolBar(_toolBar);
  setCentralWidget(docView);
  
  QDockWidget * toc = docView->dockWidget(QtPDF::PDFDocumentView::Dock_TableOfContents, this);
  addDockWidget(Qt::LeftDockWidgetArea, toc);
  tabifyDockWidget(toc, docView->dockWidget(QtPDF::PDFDocumentView::Dock_MetaData, this));
  tabifyDockWidget(toc, docView->dockWidget(QtPDF::PDFDocumentView::Dock_Fonts, this));
  tabifyDockWidget(toc, docView->dockWidget(QtPDF::PDFDocumentView::Dock_Permissions, this));
  tabifyDockWidget(toc, docView->dockWidget(QtPDF::PDFDocumentView::Dock_Annotations, this));
  toc->raise();
}

void PDFViewer::open()
{
  QString pdf_doc = QFileDialog::getOpenFileName(this, tr("Open PDF Document"), QString(), tr("PDF documents (*.pdf)"));
  if (pdf_doc.isEmpty())
    return;

  QtPDF::PDFDocumentView * docView = qobject_cast<QtPDF::PDFDocumentView*>(centralWidget());
  Q_ASSERT(docView != NULL);

#ifdef USE_MUPDF
  QtPDF::Backend::Document *a_pdf_doc = new QtPDF::Backend::MuPDF::Document(pdf_doc);
#elif USE_POPPLER
  QtPDF::Backend::Document *a_pdf_doc = new QtPDF::Backend::Poppler::Document(pdf_doc);
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

  if (a_pdf_doc && a_pdf_doc->isValid()) {
    // Note: Don't pass `this` (or any other QObject*) as parent to the new
    // PDFDocumentScene as that would cause docScene to be destroyed with its
    // parent, thereby bypassing the QSharedPointer mechanism. docScene will be
    // freed automagically when the last QSharedPointer pointing to it will be
    // destroyed.
    QSharedPointer<QtPDF::PDFDocumentScene> docScene(new QtPDF::PDFDocumentScene(a_pdf_doc));
    docView->setScene(docScene);
  }
  else
    docView->setScene(QSharedPointer<QtPDF::PDFDocumentScene>());
}

void PDFViewer::documentChanged(const QSharedPointer<QtPDF::Backend::Document> newDoc)
{
  if (_counter)
    _counter->setLastPage(newDoc->numPages());
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

void PDFViewer::openPdf(QString filename, int page, bool newWindow) const
{
  qDebug() << "Open PDF" << filename << "on page" << (page + 1) << "(in new window =" << newWindow << ")";
}

void PDFViewer::syncFromPdf(const int page, const QPointF pos)
{
  qDebug() << "Invoke SyncTeX from page" << (page + 1) << "at" << pos;
}


PageCounter::PageCounter(QWidget *parent, Qt::WindowFlags f) : super(parent, f),
  currentPage(1),
  lastPage(-1)
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
    setText(QString::fromAscii(""));
  update();
}


ZoomTracker::ZoomTracker(QWidget *parent, Qt::WindowFlags f) : super(parent, f),
  zoom(1.0)
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
