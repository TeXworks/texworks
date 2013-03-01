#include "PDFViewer.h"
#include "PDFDocumentView.h"

PDFViewer::PDFViewer(const QString pdf_doc, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags)
{
#ifdef USE_MUPDF
  Document *a_pdf_doc = new MuPDFDocument(pdf_doc);
#elif USE_POPPLER
  Document *a_pdf_doc = new PopplerDocument(pdf_doc);
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

  PDFDocumentView *docView = new PDFDocumentView(this);

  if (a_pdf_doc) {
    PDFDocumentScene *docScene = new PDFDocumentScene(a_pdf_doc, this);
    docView->setScene(docScene);
  }
  docView->goFirst();

  PageCounter *counter = new PageCounter(this->statusBar());
  ZoomTracker *zoomWdgt = new ZoomTracker(this);
  SearchLineEdit *search = new SearchLineEdit(this);
  QToolBar *toolBar = new QToolBar(this);


  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/document-open.png")), tr("Open..."), this, SLOT(open()));
  toolBar->addSeparator();

  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoomin.png")), tr("Zoom In"), docView, SLOT(zoomIn()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoomout.png")), tr("Zoom Out"), docView, SLOT(zoomOut()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/fitwidth.png")), tr("Fit to Width"), docView, SLOT(zoomFitWidth()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/fitwindow.png")), tr("Fit to Window"), docView, SLOT(zoomFitWindow()));

  toolBar->addSeparator();
  toolBar->addAction(tr("Single"), docView, SLOT(setSinglePageMode()));
  toolBar->addAction(tr("1Col Cont"), docView, SLOT(setOneColContPageMode()));
  toolBar->addAction(tr("2Col Cont"), docView, SLOT(setTwoColContPageMode()));

  toolBar->addSeparator();
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoom.png")), tr("Magnify"), docView, SLOT(setMouseModeMagnifyingGlass()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/hand.png")), tr("Pan"), docView, SLOT(setMouseModeMove()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoom-select.png")), tr("Marquee Zoom"), docView, SLOT(setMouseModeMarqueeZoom()));

  counter->setLastPage(docView->lastPage());
  connect(docView, SIGNAL(changedPage(int)), counter, SLOT(setCurrentPage(int)));
  connect(docView, SIGNAL(changedZoom(qreal)), zoomWdgt, SLOT(setZoom(qreal)));
  connect(docView, SIGNAL(requestOpenUrl(const QUrl)), this, SLOT(openUrl(const QUrl)));
  connect(docView, SIGNAL(requestOpenPdf(QString, int, bool)), this, SLOT(openPdf(QString, int, bool)));
  connect(docView, SIGNAL(contextClick(const int, const QPointF)), this, SLOT(syncFromPdf(const int, const QPointF)));

  toolBar->addSeparator();
  toolBar->addWidget(search);
  connect(search, SIGNAL(searchRequested(QString)), docView, SLOT(search(QString)));
  connect(search, SIGNAL(gotoNextResult()), docView, SLOT(nextSearchResult()));
  connect(search, SIGNAL(gotoPreviousResult()), docView, SLOT(previousSearchResult()));
  connect(search, SIGNAL(searchCleared()), docView, SLOT(clearSearchResults()));

  statusBar()->addPermanentWidget(counter);
  statusBar()->addWidget(zoomWdgt);
  addToolBar(toolBar);
  setCentralWidget(docView);
  
  QDockWidget * toc = docView->tocDockWidget(this);
  addDockWidget(Qt::LeftDockWidgetArea, toc);
  tabifyDockWidget(toc, docView->metaDataDockWidget(this));
  tabifyDockWidget(toc, docView->fontsDockWidget(this));
  toc->raise();
}

void PDFViewer::open()
{
  QString pdf_doc = QFileDialog::getOpenFileName(this, tr("Open PDF Document"), QString(), tr("PDF documents (*.pdf)"));
  if (pdf_doc.isEmpty())
    return;

  PDFDocumentView * docView = qobject_cast<PDFDocumentView*>(centralWidget());
  Q_ASSERT(docView != NULL);

#ifdef USE_MUPDF
  Document *a_pdf_doc = new MuPDFDocument(pdf_doc);
#elif USE_POPPLER
  Document *a_pdf_doc = new PopplerDocument(pdf_doc);
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

  if (a_pdf_doc && a_pdf_doc->isValid()) {
    // FIXME: Destroy old scene if necessary; use QSharedPointer for that
    PDFDocumentScene * docScene = new PDFDocumentScene(a_pdf_doc, this);
    docView->setScene(docScene);
  }
  else
    docView->setScene((PDFDocumentScene*)NULL);
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
  setText(tr("Page %1 of %2").arg(currentPage).arg(lastPage));
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
  previousResultButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  connect(previousResultButton, SIGNAL(clicked()), this, SLOT(handlePreviousResult()));

  nextResultButton = new QToolButton(this);
  nextResultButton->setIcon(style()->standardIcon(QStyle::SP_ArrowRight));
  nextResultButton->setCursor(Qt::ArrowCursor);
  nextResultButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  connect(nextResultButton, SIGNAL(clicked()), this, SLOT(handleNextResult()));

  clearButton = new QToolButton(this);
  clearButton->setIcon(style()->standardIcon(QStyle::SP_TitleBarCloseButton));
  clearButton->setCursor(Qt::ArrowCursor);
  clearButton->setStyleSheet("QToolButton { border: none; padding: 0px; }");
  connect(clearButton, SIGNAL(clicked()), this, SLOT(clearSearch()));

  int frameWidth = style()->pixelMetric(QStyle::PM_DefaultFrameWidth);
  setStyleSheet(QString("QLineEdit { padding-right: %1px; } ").arg(
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
