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

  toolBar->addAction(QIcon(":/icons/zoomin.png"), "Zoom In", docView, SLOT(zoomIn()));
  toolBar->addAction(QIcon(":/icons/zoomout.png"), "Zoom Out", docView, SLOT(zoomOut()));
  toolBar->addSeparator();
//  toolBar->addAction(tr("Single"), docView, SLOT(setSinglePageMode()));
  toolBar->addAction(tr("1Col Cont"), docView, SLOT(setOneColContPageMode()));
  toolBar->addAction(tr("2Col Cont"), docView, SLOT(setTwoColContPageMode()));
  counter->setLastPage(docView->lastPage());
  connect(docView, SIGNAL(changedPage(int)), counter, SLOT(setCurrentPage(int)));
  connect(docView, SIGNAL(changedZoom(qreal)), zoomWdgt, SLOT(setZoom(qreal)));

  statusBar()->addPermanentWidget(counter);
  statusBar()->addWidget(zoomWdgt);
  addToolBar(toolBar);
  setCentralWidget(docView);
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
  setText(QString("Page %1 of %2").arg(currentPage).arg(lastPage));
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
  setText(QString("Zoom %1%").arg(zoom * 100));
  update();
}

// vim: set sw=2 ts=2 et
