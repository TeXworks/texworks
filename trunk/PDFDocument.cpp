#include "PDFDocument.h"
#include "QTeXApp.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QPainter>
#include <QPaintEngine>
#include <QLabel>
#include <QScrollArea>
#include <QStyle>

#include <math.h>

const double kMaxScaleFactor = 8.0;
const double kMinScaleFactor = 0.125;

const int kMagnifierWidth = 360;
const int kMagnifierHeight = 240;

PDFMagnifier::PDFMagnifier(QWidget *parent)
	: QLabel(parent)
	, page(NULL)
	, scaleFactor(1.0)
{
}

void PDFMagnifier::setPage(Poppler::Page *p, double scale)
{
	page = p;
	scaleFactor = scale;
	update();
}

void PDFMagnifier::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    drawFrame(&painter);

	QImage img = page->renderToImage(72.0 * scaleFactor * 2, 72.0 * scaleFactor * 2,
									x() * 2 + event->rect().x() + kMagnifierWidth / 2,
									y() * 2 + event->rect().y() + kMagnifierHeight / 2,
									event->rect().width(), event->rect().height());
	painter.drawImage(event->rect(), img);
}


PDFWidget::PDFWidget()
	: QLabel()
	, document(NULL)
	, page(NULL)
	, pageIndex(0)
	, scaleFactor(1.0)
	, scaleOption(kFixedMag)
	, magnifier(NULL)
{
}

void PDFWidget::setDocument(Poppler::Document *doc)
{
	document = doc;
	reloadPage();
}

void PDFWidget::windowResized()
{
	switch (scaleOption) {
		case kFixedMag:
			break;
		case kFitWidth:
			fitWidth();
			break;
		case kFitWindow:
			fitWindow();
			break;
	}
}

void PDFWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    drawFrame(&painter);

	QImage img = page->renderToImage(72.0 * scaleFactor, 72.0 * scaleFactor,
									event->rect().x(), event->rect().y(),
									event->rect().width(), event->rect().height());
	painter.drawImage(event->rect(), img);
}

void PDFWidget::mousePressEvent(QMouseEvent *event)
{
	if (!magnifier)
		magnifier = new PDFMagnifier(this);
	magnifier->setPage(page, scaleFactor);
	magnifier->setFixedSize(kMagnifierWidth, kMagnifierHeight);
	magnifier->move(event->x() - kMagnifierWidth/2, event->y() - kMagnifierHeight/2);
	magnifier->show();
	event->accept();
}

void PDFWidget::mouseReleaseEvent(QMouseEvent *event)
{
	if (magnifier)
		magnifier->close();
	event->accept();
}

void PDFWidget::mouseMoveEvent(QMouseEvent *event)
{
	if (magnifier)
		magnifier->move(event->x() - kMagnifierWidth/2, event->y() - kMagnifierHeight/2);
	event->accept();
}

void PDFWidget::adjustSize()
{
	if (page) {
		QSize	pageSize = (page->pageSizeF() * scaleFactor).toSize();
		if (pageSize != size())
			resize(pageSize);
	}
}

void PDFWidget::reloadPage()
{
	if (page != NULL)
		delete page;
	page = document->page(pageIndex);
	adjustSize();
	updateStatusBar();
}

void PDFWidget::updateStatusBar()
{
	QWidget *widget = window();
	PDFDocument *doc = qobject_cast<PDFDocument*>(widget);
	if (doc)
		doc->statusBar()->showMessage(tr("%1%  page %2 of %3")
										.arg(round(scaleFactor * 10000.0) / 100.0)
										.arg(pageIndex+1)
										.arg(document->numPages())
										);
}

void PDFWidget::goFirst()
{
	if (pageIndex != 0) {
		pageIndex = 0;
		reloadPage();
		update();
	}
}

void PDFWidget::goPrev()
{
	if (pageIndex > 0) {
		--pageIndex;
		reloadPage();
		update();
	}
}

void PDFWidget::goNext()
{
	if (pageIndex < document->numPages() - 1) {
		++pageIndex;
		reloadPage();
		update();
	}
}

void PDFWidget::goLast()
{
	if (pageIndex != document->numPages() - 1) {
		pageIndex = document->numPages() - 1;
		reloadPage();
		update();
	}
}

void PDFWidget::doPageDialog()
{
}


void PDFWidget::actualSize()
{
	scaleOption = kFixedMag;
	if (scaleFactor != 1.0) {
		scaleFactor = 1.0;
		adjustSize();
		update();
		updateStatusBar();
	}
}

void PDFWidget::fitWidth()
{
	scaleOption = kFitWidth;
	QWidget *widget = window();
	PDFDocument*	doc = qobject_cast<PDFDocument*>(widget);
	if (doc) {
		QScrollArea*	scrollArea = qobject_cast<QScrollArea*>(doc->centralWidget());
		if (scrollArea) {
			double portWidth = scrollArea->viewport()->width();
			QSizeF	pageSize = page->pageSizeF();
			scaleFactor = portWidth / pageSize.width();
			if (scaleFactor < kMinScaleFactor)
				scaleFactor = kMinScaleFactor;
			else if (scaleFactor > kMaxScaleFactor)
				scaleFactor = kMaxScaleFactor;
			adjustSize();
			update();
			updateStatusBar();
		}
	}
}

void PDFWidget::fitWindow()
{
	scaleOption = kFitWindow;
	QWidget *widget = window();
	PDFDocument*	doc = qobject_cast<PDFDocument*>(widget);
	if (doc) {
		QScrollArea*	scrollArea = qobject_cast<QScrollArea*>(doc->centralWidget());
		if (scrollArea) {
			double portWidth = scrollArea->viewport()->width();
			double portHeight = scrollArea->viewport()->height();
			QSizeF	pageSize = page->pageSizeF();
			double sfh = portWidth / pageSize.width();
			double sfv = portHeight / pageSize.height();
			scaleFactor = sfh < sfv ? sfh : sfv;
			if (scaleFactor < kMinScaleFactor)
				scaleFactor = kMinScaleFactor;
			else if (scaleFactor > kMaxScaleFactor)
				scaleFactor = kMaxScaleFactor;
			adjustSize();
			update();
			updateStatusBar();
		}
	}
}

void PDFWidget::zoomIn()
{
	scaleOption = kFixedMag;
	if (scaleFactor < kMaxScaleFactor) {
		scaleFactor *= sqrt(2.0);
		if (fabs(scaleFactor - round(scaleFactor)) < 0.01)
			scaleFactor = round(scaleFactor);
		if (scaleFactor > kMaxScaleFactor)
			scaleFactor = kMaxScaleFactor;
		adjustSize();
		update();
		updateStatusBar();
	}
}

void PDFWidget::zoomOut()
{
	scaleOption = kFixedMag;
	if (scaleFactor > kMinScaleFactor) {
		scaleFactor /= sqrt(2.0);
		if (fabs(scaleFactor - round(scaleFactor)) < 0.01)
			scaleFactor = round(scaleFactor);
		if (scaleFactor < kMinScaleFactor)
			scaleFactor = kMinScaleFactor;
		adjustSize();
		update();
		updateStatusBar();
	}
}

const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

PDFDocument::PDFDocument(const QString &fileName, TeXDocument *texDoc)
	: sourceDoc(texDoc)
{
	init();
	loadFile(fileName);
}

void
PDFDocument::init()
{
	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_MacNoClickThrough, true);
	
	pdfWidget = new PDFWidget;
	pdfWidget->setBackgroundRole(QPalette::Base);
	pdfWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	pdfWidget->setScaledContents(true);
	connect(this, SIGNAL(windowResized()), pdfWidget, SLOT(windowResized()));

	scrollArea = new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setWidget(pdfWidget);
	setCentralWidget(scrollArea);

	document = NULL;
	
	connect(actionAbout_QTeX, SIGNAL(triggered()), qApp, SLOT(about()));

	connect(actionFirst_Page, SIGNAL(triggered()), pdfWidget, SLOT(goFirst()));
	connect(actionPrevious_Page, SIGNAL(triggered()), pdfWidget, SLOT(goPrev()));
	connect(actionNext_Page, SIGNAL(triggered()), pdfWidget, SLOT(goNext()));
	connect(actionLast_Page, SIGNAL(triggered()), pdfWidget, SLOT(goLast()));

	connect(actionActual_Size, SIGNAL(triggered()), pdfWidget, SLOT(actualSize()));
	connect(actionFit_to_Width, SIGNAL(triggered()), pdfWidget, SLOT(fitWidth()));
	connect(actionFit_to_Window, SIGNAL(triggered()), pdfWidget, SLOT(fitWindow()));
	connect(actionZoom_In, SIGNAL(triggered()), pdfWidget, SLOT(zoomIn()));
	connect(actionZoom_Out, SIGNAL(triggered()), pdfWidget, SLOT(zoomOut()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->insertMenu(actionOpen_Recent, menuRecent);
	menuFile->removeAction(actionOpen_Recent);

	connect(qApp, SIGNAL(recentFileActionsChanged()), this, SLOT(updateRecentFileActions()));
}
 
void PDFDocument::updateRecentFileActions()
{
	QTeXApp::updateRecentFileActions(this, recentFileActions, menuRecent);
}

//void PDFDocument::closeEvent(QCloseEvent *event)
//{
//	event->accept();
//}

void PDFDocument::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	emit windowResized();
}

void PDFDocument::loadFile(const QString &fileName)
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	document = Poppler::Document::load(fileName);
	document->setRenderBackend(Poppler::Document::SplashBackend);
	document->setRenderHint(Poppler::Document::Antialiasing);
	document->setRenderHint(Poppler::Document::TextAntialiasing);
	pdfWidget->setDocument(document);
	
	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);

	if (document == NULL) {
		statusBar()->showMessage(tr("Failed to load file \"%1\"").arg(strippedName(curFile)));
	}
}

void PDFDocument::setCurrentFile(const QString &fileName)
{
	curFile = QFileInfo(fileName).canonicalFilePath();

	setWindowTitle(tr("%1[*] - %2").arg(strippedName(curFile)).arg(tr("TeXWorks")));
}
 
QString PDFDocument::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

PDFDocument *PDFDocument::findDocument(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QWidget *widget, qApp->topLevelWidgets()) {
		PDFDocument *theDoc = qobject_cast<PDFDocument*>(widget);
		if (theDoc && theDoc->curFile == canonicalFilePath)
			return theDoc;
	}
	return NULL;
}
