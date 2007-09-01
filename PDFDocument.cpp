#include "PDFDocument.h"

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

PDFWidget::PDFWidget()
	: QLabel()
	, document(NULL)
	, page(NULL)
	, pageIndex(0)
{
}

void
PDFWidget::setDocument(Poppler::Document *doc)
{
	document = doc;
	pageIndex = 0;
	scaleFactor = 1.0;
	reloadPage();
}

void
PDFWidget::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    drawFrame(&painter);

	QImage img = page->renderToImage(72.0 * scaleFactor, 72.0 * scaleFactor,
									event->rect().x(), event->rect().y(),
									event->rect().width(), event->rect().height());
	painter.drawImage(event->rect(), img);
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
	PDFDocument *doc = qobject_cast<PDFDocument *>(widget);
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
	if (scaleFactor != 1.0) {
		scaleFactor = 1.0;
		adjustSize();
		update();
		updateStatusBar();
	}
}

void PDFWidget::fitWidth()
{
	QWidget *widget = window();
	PDFDocument*	doc = qobject_cast<PDFDocument *>(widget);
	if (doc) {
		QScrollArea*	scrollArea = qobject_cast<QScrollArea *>(doc->centralWidget());
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
}

void PDFWidget::zoomIn()
{
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

PDFDocument::PDFDocument(const QString &fileName)
{
	init();
	loadFile(fileName);
}

void
PDFDocument::init()
{
	setupUi(this);
	
	pdfWidget = new PDFWidget;
	pdfWidget->setBackgroundRole(QPalette::Base);
	pdfWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	pdfWidget->setScaledContents(true);

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
}
 
void PDFDocument::closeEvent(QCloseEvent *event)
{
	event->accept();
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

	setWindowTitle(tr("%1[*] - %2").arg(strippedName(curFile)).arg(tr("QTeX")));
}
 
QString PDFDocument::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

PDFDocument *PDFDocument::findDocument(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QWidget *widget, qApp->topLevelWidgets()) {
		PDFDocument *theDoc = qobject_cast<PDFDocument *>(widget);
		if (theDoc && theDoc->curFile == canonicalFilePath)
			return theDoc;
	}
	return NULL;
}
