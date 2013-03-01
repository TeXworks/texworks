#include "PDFViewer.h"
#include "PDFDocumentView.h"

PDFViewer::PDFViewer(QString pdf_doc, QWidget *parent, Qt::WindowFlags flags) :
  QMainWindow(parent, flags)
{
  Poppler::Document *pdf = Poppler::Document::load(pdf_doc);
  PDFDocumentScene *docScene = new PDFDocumentScene(pdf, this);
  PDFDocumentView *docView = new PDFDocumentView(this);

  docView->setScene(docScene);
  docView->goFirst();

  PageCounter *counter = new PageCounter(this->statusBar());
  ZoomTracker *zoomWdgt = new ZoomTracker(this);
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
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/zoom.png")), tr("Magnify"), docView, SLOT(setMouseModeMagnify()));
  toolBar->addAction(QIcon(QString::fromUtf8(":/icons/hand.png")), tr("Pan"), docView, SLOT(setMouseModePan()));

  counter->setLastPage(docView->lastPage());
  connect(docView, SIGNAL(changedPage(int)), counter, SLOT(setCurrentPage(int)));
  connect(docView, SIGNAL(changedZoom(qreal)), zoomWdgt, SLOT(setZoom(qreal)));
  connect(docView, SIGNAL(requestOpenUrl(const QUrl)), this, SLOT(openUrl(const QUrl)));

  statusBar()->addPermanentWidget(counter);
  statusBar()->addWidget(zoomWdgt);
  addToolBar(toolBar);
  setCentralWidget(docView);
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

// vim: set sw=2 ts=2 et