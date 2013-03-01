#include "PDFViewer.h"
#include "PDFDocumentView.h"

PDFViewer::PDFViewer(QString pdf_doc, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags)
{
#ifdef USE_MUPDF
  Document *a_pdf_doc = new MuPDFDocument(pdf_doc);
#elif USE_POPPLER
  Document *a_pdf_doc = new PopplerDocument(pdf_doc);
#else
  #error Either the Poppler or the MuPDF backend is required
#endif

  PDFDocumentScene *docScene = new PDFDocumentScene(a_pdf_doc, this);
  PDFDocumentView *docView = new PDFDocumentView(this);

  docView->setScene(docScene);
  docView->goFirst();

  PageCounter *counter = new PageCounter(this->statusBar());
  ZoomTracker *zoomWdgt = new ZoomTracker(this);
  SearchWidget *search = new SearchWidget(this);
  QToolBar *toolBar = new QToolBar(this);


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


SearchWidget::SearchWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
  setLayout(new QHBoxLayout());

  _input = new QLineEdit(this);
  _searchButton = new QPushButton(QString::fromAscii("Search"), this);

  layout()->addWidget(_input);
  layout()->addWidget(_searchButton);

  connect(_searchButton, SIGNAL(clicked()), this, SLOT(searchActivated()));
  connect(_input, SIGNAL(returnPressed()), this, SLOT(searchActivated()));

}

void SearchWidget::searchActivated() {
  if( _input->text().isEmpty() )
    return;

  emit(searchRequested(_input->text()));
}


// vim: set sw=2 ts=2 et
