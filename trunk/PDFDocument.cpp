#include "PDFDocument.h"
#include "TeXDocument.h"
#include "QTeXApp.h"
#include "QTeXUtils.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QStatusBar>
#include <QPainter>
#include <QPaintEngine>
#include <QLabel>
#include <QScrollArea>
#include <QStyle>
#include <QDesktopWidget>
#include <QSettings>
#include <QScrollBar>
#include <QRegion>
#include <QVector>
#include <QHash>
#include <QList>
#include <QStack>

#include <math.h>

#include "GlobalParams.h"

#define SYNCTEX_EXT		".synctex"

const double kMaxScaleFactor = 8.0;
const double kMinScaleFactor = 0.125;

const int magSizes[] = { 200, 300, 400 };

// tool codes
const int kNone = 0;
const int kMagnifier = 1;
const int kScroll = 2;
const int kSelectText = 3;
const int kSelectImage = 4;

#pragma mark === PDFMagnifier ===

const int kMagFactor = 2;

PDFMagnifier::PDFMagnifier(QWidget *parent, double inDpi)
	: QLabel(parent)
	, page(NULL)
	, scaleFactor(kMagFactor)
	, dpi(inDpi)
	, imageDpi(0)
	, imagePage(NULL)
{
}

void PDFMagnifier::setPage(Poppler::Page *p, double scale)
{
	page = p;
	scaleFactor = scale * kMagFactor;
	if (page == NULL) {
		imagePage = NULL;
		image = QImage();
	}
	else {
		QWidget *parentWidget = qobject_cast<QWidget*>(parent());
		double newDpi = dpi * scaleFactor;
		QPoint newLoc = parentWidget->rect().topLeft() * kMagFactor;
		QSize newSize = parentWidget->rect().size() * kMagFactor;
		if (page != imagePage || newDpi != imageDpi || newLoc != imageLoc || newSize != imageSize)
			image = page->renderToImage(newDpi, newDpi,
										newLoc.x(), newLoc.y(),
										newSize.width(), newSize.height());
		imagePage = page;
		imageDpi = newDpi;
		imageLoc = newLoc;
		imageSize = newSize;
	}
	update();
}

void PDFMagnifier::paintEvent(QPaintEvent *event)
{
    QPainter painter(this);
    drawFrame(&painter);
	painter.drawImage(event->rect(), image,
		event->rect().translated(x() * kMagFactor + width() / 2,
								 y() * kMagFactor + height() / 2));
}

void PDFMagnifier::resizeEvent(QResizeEvent * /*event*/)
{
	QSettings settings;
	if (settings.value("circularMagnifier", kDefault_CircularMagnifier).toBool()) {
		int side = qMin(width(), height());
		QRegion maskedRegion(width() / 2 - side / 2, height() / 2 - side / 2, side, side, QRegion::Ellipse);
		setMask(maskedRegion);
	}
}

#pragma mark === PDFWidget ===

QCursor *PDFWidget::magnifierCursor = NULL;
QCursor *PDFWidget::zoomInCursor = NULL;
QCursor *PDFWidget::zoomOutCursor = NULL;

PDFWidget::PDFWidget()
	: QLabel()
	, document(NULL)
	, page(NULL)
	, pageIndex(0)
	, scaleFactor(1.0)
	, dpi(72.0)
	, scaleOption(kFixedMag)
	, magnifier(NULL)
	, usingTool(kNone)
{
	QSettings settings;
	dpi = settings.value("previewResolution", QApplication::desktop()->logicalDpiX()).toInt();
	
	setBackgroundRole(QPalette::Base);
	setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
	setFocusPolicy(Qt::StrongFocus);
	setScaledContents(true);

	if (magnifierCursor == NULL) {
		magnifierCursor = new QCursor(QPixmap(":/images/images/magnifiercursor.png"));
		zoomInCursor = new QCursor(QPixmap(":/images/images/zoomincursor.png"));
		zoomOutCursor = new QCursor(QPixmap(":/images/images/zoomoutcursor.png"));
	}
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
			fitWidth(true);
			break;
		case kFitWindow:
			fitWindow(true);
			break;
	}
}

void PDFWidget::paintEvent(QPaintEvent *event)
{
	QPainter painter(this);
	drawFrame(&painter);

	double newDpi = dpi * scaleFactor;
	QRect newRect = rect();
	if (page != imagePage || newDpi != imageDpi || newRect != imageRect)
		image = page->renderToImage(dpi * scaleFactor, dpi * scaleFactor,
									rect().x(), rect().y(),
									rect().width(), rect().height());
	imagePage = page;
	imageDpi = newDpi;
	imageRect = newRect;

	painter.drawImage(event->rect(), image, event->rect());

	if (!highlightBoxes.isEmpty()) {
		painter.setRenderHint(QPainter::Antialiasing);
		painter.setCompositionMode(QPainter::CompositionMode_ColorBurn);
		painter.scale(72 / 72.27 * scaleFactor / 8, 72 / 72.27 * scaleFactor / 8);
		painter.setPen(QColor(0, 0, 0, 0));
		painter.setBrush(QColor(255, 255, 0, 63));
		
		foreach (const QRectF& box, highlightBoxes)
			painter.drawRoundRect(box);
	}
}

void PDFWidget::useMagnifier(const QMouseEvent *inEvent)
{
	if (!magnifier) {
		magnifier = new PDFMagnifier(this, dpi);
		QSettings settings;
		int magnifierSize = settings.value("magnifierSize", kDefault_MagnifierSize).toInt();
		if (magnifierSize <= 0 || magnifierSize > (int)(sizeof(magSizes) / sizeof(int)))
			magnifierSize = kDefault_MagnifierSize;
		magnifierSize = magSizes[magnifierSize - 1];
		magnifier->setFixedSize(magnifierSize * 4 / 3, magnifierSize);
	}
	magnifier->setPage(page, scaleFactor);
	// this was in the hope that if the mouse is released before the image is ready,
	// the magnifier wouldn't actually get shown. but it doesn't seem to work that way -
	// the MouseMove event that we're posting must end up ahead of the mouseUp
	QMouseEvent *event = new QMouseEvent(QEvent::MouseMove, inEvent->pos(), inEvent->globalPos(), inEvent->button(), inEvent->buttons(), inEvent->modifiers());
	QCoreApplication::postEvent(this, event);
	usingTool = kMagnifier;
}

// Mouse control for the various tools:
// * magnifier
//   - ctrl-click to sync
//   - click to use magnifier
//   - shift-click to zoom in
//   - shift-click and drag to zoom to selected area
//   - alt-click to zoom out
// * scroll (hand)
//   - ctrl-click to sync
//   - click and drag to scroll
//   - double-click to use magnifier
// * select area (crosshair)
//   - ctrl-click to sync
//   - click and drag to select area
//   - double-click to use magnifier
// * select text (I-beam)
//   - ctrl-click to sync
//   - click and drag to select text
//   - double-click to use magnifier

static QPoint scrollClickPos;
static Qt::KeyboardModifiers mouseDownModifiers;

void PDFWidget::mousePressEvent(QMouseEvent *event)
{
	mouseDownModifiers = event->modifiers();
	if (mouseDownModifiers & Qt::ControlModifier) {
		// ctrl key - this is a sync click, don't handle the mouseDown here
	}
	else switch (currentTool) {
		case kMagnifier:
			if (mouseDownModifiers & (Qt::ShiftModifier | Qt::AltModifier))
				; // do nothing - zoom in or out (on mouseUp)
			else
				useMagnifier(event);
			break;
		
		case kScroll:
			setCursor(Qt::ClosedHandCursor);
			scrollClickPos = event->globalPos();
			usingTool = kScroll;
			break;
	}
	event->accept();
}

void PDFWidget::mouseReleaseEvent(QMouseEvent *event)
{
	switch (usingTool) {
		case kNone:
			if (mouseDownModifiers & Qt::ControlModifier) {
				if (event->modifiers() & Qt::ControlModifier)
					emit syncClick(pageIndex, scaleFactor, event->pos());
				break;
			}
			if (currentTool == kMagnifier) {
				Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
				if (mods & Qt::AltModifier)
					zoomOut();
				else if (mods & Qt::ShiftModifier)
					zoomIn();
			}
			break;
		case kMagnifier:
			magnifier->close();
			break;
	}

	usingTool = kNone;
	updateCursor();
	event->accept();
}

void PDFWidget::mouseDoubleClickEvent(QMouseEvent *event)
{
	useMagnifier(event);
	event->accept();
}

void PDFWidget::mouseMoveEvent(QMouseEvent *event)
{
	switch (usingTool) {
		case kMagnifier:
			magnifier->move(event->x() - magnifier->width() / 2, event->y() - magnifier->height() / 2);
			if (magnifier->isHidden()) {
				magnifier->show();
				setCursor(Qt::BlankCursor);
			}
			break;

		case kScroll:
			{
				QPoint delta = event->globalPos() - scrollClickPos;
				scrollClickPos = event->globalPos();
				QWidget *widget = window();
				PDFDocument*	doc = qobject_cast<PDFDocument*>(widget);
				if (doc) {
					QScrollArea*	scrollArea = qobject_cast<QScrollArea*>(doc->centralWidget());
					if (scrollArea) {
						int oldX = scrollArea->horizontalScrollBar()->value();
						scrollArea->horizontalScrollBar()->setValue(oldX - delta.x());
						int oldY = scrollArea->verticalScrollBar()->value();
						scrollArea->verticalScrollBar()->setValue(oldY - delta.y());
					}
				}
			}
			break;
	}
	event->accept();
}

void PDFWidget::keyPressEvent(QKeyEvent *event)
{
	updateCursor();
	event->ignore();
}

void PDFWidget::keyReleaseEvent(QKeyEvent *event)
{
	updateCursor();
	event->ignore();
}

void PDFWidget::focusInEvent(QFocusEvent *event)
{
	updateCursor();
	event->ignore();
}

void PDFWidget::setTool(int tool)
{
	currentTool = tool;
	updateCursor();
}

void PDFWidget::updateCursor()
{
	if (usingTool != kNone)
		return;
	switch (currentTool) {
		case kScroll:
			setCursor(Qt::OpenHandCursor);
			break;
		case kMagnifier:
			{
				Qt::KeyboardModifiers mods = QApplication::keyboardModifiers();
				if (mods & Qt::AltModifier)
					setCursor(*zoomOutCursor);
				else if (mods & Qt::ShiftModifier)
					setCursor(*zoomInCursor);
				else
					setCursor(*magnifierCursor);
			}
			break;
		case kSelectText:
			setCursor(Qt::IBeamCursor);
			break;
		case kSelectImage:
			setCursor(Qt::CrossCursor);
			break;
	}
}

void PDFWidget::adjustSize()
{
	if (page) {
		QSize	pageSize = (page->pageSizeF() * scaleFactor * dpi / 72.0).toSize();
		if (pageSize != size())
			resize(pageSize);
	}
}

void PDFWidget::resetMagnifier()
{
	if (magnifier) {
		delete magnifier;
		magnifier = NULL;
	}
}

void PDFWidget::setResolution(int res)
{
	dpi = res;
	adjustSize();
	resetMagnifier();
}

void PDFWidget::setHighlightBoxes(const QList<QRectF>& boxlist)
{
	highlightBoxes = boxlist;
}

void PDFWidget::reloadPage()
{
	if (page != NULL)
		delete page;
	if (magnifier != NULL)
		magnifier->setPage(NULL, 0);
	imagePage = NULL;
	image = QImage();
	highlightBoxes.clear();
	page = document->page(pageIndex);
	adjustSize();
	update();
	updateStatusBar();
	emit changedPage(pageIndex);
}

void PDFWidget::updateStatusBar()
{
	QWidget *widget = window();
	PDFDocument *doc = qobject_cast<PDFDocument*>(widget);
	if (doc) {
		doc->showPage(pageIndex+1);
		doc->showScale(scaleFactor);
	}
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

void PDFWidget::goToPage(int p)
{
	if (p != pageIndex) {
		if (p > 0 && p < document->numPages()) {
			pageIndex = p;
			reloadPage();
			update();
		}
	}
}

void PDFWidget::actualSize()
{
	scaleOption = kFixedMag;
	if (scaleFactor != 1.0) {
		scaleFactor = 1.0;
		adjustSize();
		update();
		updateStatusBar();
		emit changedZoom(scaleFactor);
	}
	emit changedScaleOption(scaleOption);
}

void PDFWidget::fitWidth(bool checked)
{
	if (checked) {
		scaleOption = kFitWidth;
		QWidget *widget = window();
		PDFDocument*	doc = qobject_cast<PDFDocument*>(widget);
		if (doc) {
			QScrollArea*	scrollArea = qobject_cast<QScrollArea*>(doc->centralWidget());
			if (scrollArea) {
				double portWidth = scrollArea->viewport()->width();
				QSizeF	pageSize = page->pageSizeF() * dpi / 72.0;
				scaleFactor = portWidth / pageSize.width();
				if (scaleFactor < kMinScaleFactor)
					scaleFactor = kMinScaleFactor;
				else if (scaleFactor > kMaxScaleFactor)
					scaleFactor = kMaxScaleFactor;
				adjustSize();
				update();
				updateStatusBar();
				emit changedZoom(scaleFactor);
			}
		}
	}
	else
		scaleOption = kFixedMag;
	emit changedScaleOption(scaleOption);
}

void PDFWidget::fitWindow(bool checked)
{
	if (checked) {
		scaleOption = kFitWindow;
		QWidget *widget = window();
		PDFDocument*	doc = qobject_cast<PDFDocument*>(widget);
		if (doc) {
			QScrollArea*	scrollArea = qobject_cast<QScrollArea*>(doc->centralWidget());
			if (scrollArea) {
				double portWidth = scrollArea->viewport()->width();
				double portHeight = scrollArea->viewport()->height();
				QSizeF	pageSize = page->pageSizeF() * dpi / 72.0;
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
				emit changedZoom(scaleFactor);
			}
		}
	}
	else
		scaleOption = kFixedMag;
	emit changedScaleOption(scaleOption);
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
		emit changedZoom(scaleFactor);
	}
	emit changedScaleOption(scaleOption);
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
		emit changedZoom(scaleFactor);
	}
	emit changedScaleOption(scaleOption);
}

void PDFWidget::saveState()
{
	saveScaleFactor = scaleFactor;
	saveScaleOption = scaleOption;
}

void PDFWidget::restoreState()
{
	if (scaleFactor != saveScaleFactor) {
		scaleFactor = saveScaleFactor;
		adjustSize();
		update();
		updateStatusBar();
		emit changedZoom(scaleFactor);
	}
	scaleOption = saveScaleOption;
	emit changedScaleOption(scaleOption);
}


#pragma mark === PDFDocument ===

QList<PDFDocument*> PDFDocument::docList;

PDFDocument::PDFDocument(const QString &fileName, TeXDocument *texDoc)
	: sourceDoc(texDoc)
{
	init();
	loadFile(fileName);
	stackUnder((QWidget*)texDoc);
}

PDFDocument::~PDFDocument()
{
	docList.removeAll(this);
}

void
PDFDocument::init()
{
	docList.append(this);

	setupUi(this);
	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_MacNoClickThrough, true);
	setWindowIcon(QIcon(":/images/images/pdfdoc.png"));
	
	pdfWidget = new PDFWidget;
	connect(this, SIGNAL(windowResized()), pdfWidget, SLOT(windowResized()));

	toolButtonGroup = new QButtonGroup(toolBar);
	toolButtonGroup->addButton(qobject_cast<QAbstractButton*>(toolBar->widgetForAction(actionMagnify)), kMagnifier);
	toolButtonGroup->addButton(qobject_cast<QAbstractButton*>(toolBar->widgetForAction(actionScroll)), kScroll);
	toolButtonGroup->addButton(qobject_cast<QAbstractButton*>(toolBar->widgetForAction(actionSelect_Text)), kSelectText);
	toolButtonGroup->addButton(qobject_cast<QAbstractButton*>(toolBar->widgetForAction(actionSelect_Image)), kSelectImage);
	connect(toolButtonGroup, SIGNAL(buttonClicked(int)), pdfWidget, SLOT(setTool(int)));
	pdfWidget->setTool(kMagnifier);

	scaleLabel = new QLabel();
	statusBar()->addPermanentWidget(scaleLabel);
	scaleLabel->setFrameStyle(QFrame::StyledPanel);
	scaleLabel->setFont(statusBar()->font());

	pageLabel = new QLabel();
	statusBar()->addPermanentWidget(pageLabel);
	pageLabel->setFrameStyle(QFrame::StyledPanel);
	pageLabel->setFont(statusBar()->font());

	scrollArea = new QScrollArea;
	scrollArea->setBackgroundRole(QPalette::Dark);
	scrollArea->setAlignment(Qt::AlignCenter);
	scrollArea->setWidget(pdfWidget);
	setCentralWidget(scrollArea);

	document = NULL;
	
	connect(actionAbout_QTeX, SIGNAL(triggered()), qApp, SLOT(about()));

	connect(actionNew, SIGNAL(triggered()), qApp, SLOT(newFile()));
	connect(actionOpen, SIGNAL(triggered()), qApp, SLOT(open()));

	connect(actionFirst_Page, SIGNAL(triggered()), pdfWidget, SLOT(goFirst()));
	connect(actionPrevious_Page, SIGNAL(triggered()), pdfWidget, SLOT(goPrev()));
	connect(actionNext_Page, SIGNAL(triggered()), pdfWidget, SLOT(goNext()));
	connect(actionLast_Page, SIGNAL(triggered()), pdfWidget, SLOT(goLast()));
	connect(pdfWidget, SIGNAL(changedPage(int)), this, SLOT(enablePageActions(int)));

	connect(actionActual_Size, SIGNAL(triggered()), pdfWidget, SLOT(actualSize()));
	connect(actionFit_to_Width, SIGNAL(triggered(bool)), pdfWidget, SLOT(fitWidth(bool)));
	connect(actionFit_to_Window, SIGNAL(triggered(bool)), pdfWidget, SLOT(fitWindow(bool)));
	connect(actionZoom_In, SIGNAL(triggered()), pdfWidget, SLOT(zoomIn()));
	connect(actionZoom_Out, SIGNAL(triggered()), pdfWidget, SLOT(zoomOut()));
	connect(actionFull_Screen, SIGNAL(triggered()), this, SLOT(toggleFullScreen()));
	connect(pdfWidget, SIGNAL(changedZoom(double)), this, SLOT(enableZoomActions(double)));
	connect(pdfWidget, SIGNAL(changedScaleOption(autoScaleOption)), this, SLOT(adjustScaleActions(autoScaleOption)));
	connect(pdfWidget, SIGNAL(syncClick(int, double, const QPoint&)), this, SLOT(syncClick(int, double, const QPoint&)));
	
	connect(actionTypeset, SIGNAL(triggered()), this, SLOT(retypeset()));
	
	connect(actionStack, SIGNAL(triggered()), qApp, SLOT(stackWindows()));
	connect(actionTile, SIGNAL(triggered()), qApp, SLOT(tileWindows()));
	connect(actionTile_Front_Two, SIGNAL(triggered()), qApp, SLOT(tileTwoWindows()));
	connect(actionGo_to_Source, SIGNAL(triggered()), this, SLOT(goToSource()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->insertMenu(actionOpen_Recent, menuRecent);
	menuFile->removeAction(actionOpen_Recent);

	connect(qApp, SIGNAL(recentFileActionsChanged()), this, SLOT(updateRecentFileActions()));
	connect(qApp, SIGNAL(windowListChanged()), this, SLOT(updateWindowMenu()));

	connect(actionPreferences, SIGNAL(triggered()), qApp, SLOT(preferences()));

	connect(this, SIGNAL(destroyed()), qApp, SLOT(updateWindowMenus()));

	connect(qApp, SIGNAL(syncPdf(const QString&, int)), this, SLOT(syncFromSource(const QString&, int)));

	QSettings settings;
	QTeXUtils::applyToolbarOptions(this, settings.value("toolBarIconSize", 2).toInt(), settings.value("toolBarShowText", false).toBool());
}
 
void PDFDocument::updateRecentFileActions()
{
	QTeXUtils::updateRecentFileActions(this, recentFileActions, menuRecent);
}

void PDFDocument::updateWindowMenu()
{
	QTeXUtils::updateWindowMenu(this, menuWindow);
}

void PDFDocument::selectWindow()
{
	show();
	raise();
	activateWindow();
}

void PDFDocument::resizeEvent(QResizeEvent *event)
{
	QMainWindow::resizeEvent(event);
	emit windowResized();
}

void PDFDocument::loadFile(const QString &fileName)
{
	setCurrentFile(fileName);
	reload();
}

void PDFDocument::reload()
{
	QApplication::setOverrideCursor(Qt::WaitCursor);

	if (document != NULL)
		delete document;

	document = Poppler::Document::load(curFile);
	document->setRenderBackend(Poppler::Document::SplashBackend);
	document->setRenderHint(Poppler::Document::Antialiasing);
	document->setRenderHint(Poppler::Document::TextAntialiasing);
	globalParams->setScreenType(screenDispersed);

	pdfWidget->setDocument(document);
	
	QApplication::restoreOverrideCursor();

	pdfWidget->setFocus();

	if (document == NULL)
		statusBar()->showMessage(tr("Failed to load file \"%1\"").arg(QTeXUtils::strippedName(curFile)));

	if (document != NULL) {
		// FIXME: see if this takes long enough that we should offload it to a separate thread
		loadSyncData();
	}
}

#define MAX_SYNC_LINE_LENGTH	(PATH_MAX + 256)

void PDFDocument::loadSyncData()
{
	QFileInfo fi(curFile);
	QString syncName = fi.canonicalPath() + "/" + fi.completeBaseName() + SYNCTEX_EXT;
	fi.setFile(syncName);

	if (fi.exists()) {
		QFile	syncFile(syncName);
		if (syncFile.open(QIODevice::ReadOnly)) {
			int sheet = 0;
			QStack<HBox> openBoxes;
			char data[MAX_SYNC_LINE_LENGTH];
			qint64 len;
			len = syncFile.readLine(data, MAX_SYNC_LINE_LENGTH);
			data[len] = 0;
			if (strncmp(data, "SyncTeX", 7) != 0 && strncmp(data, "synchronize", 11) != 0) {
				statusBar()->showMessage(tr("Unrecognized SyncTeX header line \"%1\"").arg(data), kStatusMessageDuration);
				goto done;
			}
			len = syncFile.readLine(data, MAX_SYNC_LINE_LENGTH);
			data[len] = 0;
			if (strncmp(data, "version:1", 9) != 0) {
				statusBar()->showMessage(tr("Unrecognized SyncTeX format \"%1\"").arg(data), kStatusMessageDuration);
				goto done;
			}
			while ((len = syncFile.readLine(data, MAX_SYNC_LINE_LENGTH)) > 0) {
				data[len] = 0;
				if (len > 1 && data[1] == ':') {
					switch (data[0]) {
						case 'i':
							//i:18:42MRKUK.TEV
							{
								int tag;
								char filename[PATH_MAX];
								sscanf(data + 2, "%d:%s", &tag, filename);
								QFileInfo info(QFileInfo(curFile).absoluteDir(), filename);
								tagToFile[tag] = info.canonicalFilePath();
							}
							break;
						case 's':
							//s:1
							{
								sscanf(data + 2, "%d", &sheet);
								while (pageSyncInfo.count() < sheet)
									pageSyncInfo.append(PageSyncInfo());
								openBoxes.clear();
							}
							break;
						case 'h':
							//h:18:39(-578,3840,3368,4074)0
							if (sheet > 0) {
								int tag, line, x, y, w, h, d;
								sscanf(data + 2, "%d:%d(%d,%d,%d,%d)%d", &tag, &line, &x, &y, &w, &h, &d);
								HBox hb = { tag, line, x, y, w, h, INT_MAX, -1 };
								openBoxes.push(hb);
							}
							break;
						case 'g':
						case 'k':
						case '$':
							//g:18:39(-578,3840)
							if (sheet > 0) {
								if (!openBoxes.isEmpty()) {
									HBox& hb = openBoxes.top();
									int tag, line, x, y;
									sscanf(data + 2, "%d:%d(%d,%d)", &tag, &line, &x, &y);
									if (tag == hb.tag) {
										if (line < hb.first)
											hb.first = line;
										if (line > hb.last)
											hb.last = line;
									}
								}
							}
							break;
						default:
							break;
					}
				}
				else if (data[0] == 'e' && data[1] <= ' ') {
					if (sheet > 0) {
						PageSyncInfo& psi = pageSyncInfo[sheet - 1];
						psi.append(openBoxes.pop());
					}
				}
			}
			statusBar()->showMessage(tr("Loaded SyncTeX data: \"%1\"").arg(syncName), kStatusMessageDuration);
		done:
			syncFile.close();
		}
	}
}

void PDFDocument::syncClick(int page, double scaleFactor, const QPoint& pos)
{
	if (page < pageSyncInfo.count()) {
		const PageSyncInfo& psi = pageSyncInfo[page];
		foreach (const HBox& hb, psi) {
			QRectF r((scaleFactor * (hb.x + 72 * 8)) / 8, (scaleFactor * (hb.y - hb.h + 72 * 8)) / 8, (scaleFactor * hb.w) / 8, (scaleFactor * hb.h) / 8);
			if (r.contains(pos.x(), pos.y())) {
				TeXDocument::openDocument(tagToFile[hb.tag], (hb.first < INT_MAX) ? hb.first : hb.line);
				break;
			}
		}
	}
}

void PDFDocument::syncFromSource(const QString& sourceFile, int lineNo)
{
	int tag = -1;
	foreach (int i, tagToFile.keys()) {
		if (tagToFile[i] == sourceFile) {
			tag = i;
			break;
		}
	}
	if (tag != -1) {
		QList<QRectF> boxlist;
		int pageIndex = -1;
		for (int p = 0; pageIndex == -1 && p < pageSyncInfo.size(); ++p) {
			const PageSyncInfo& psi = pageSyncInfo[p];
			foreach (const HBox& hb, psi) {
				if (hb.tag != tag)
					continue;
				if (hb.first <= lineNo && hb.last >= lineNo) {
					if (pageIndex == -1)
						pageIndex = p;
					if (pageIndex == p)
						boxlist.append(QRectF(hb.x + 72 * 8, hb.y - hb.h + 72 * 8, hb.w, hb.h));
				}
			}
		}
		if (pageIndex != -1) {
			pdfWidget->goToPage(pageIndex);
			pdfWidget->setHighlightBoxes(boxlist);
			pdfWidget->update();
			selectWindow();
		}
	}
}

void PDFDocument::setCurrentFile(const QString &fileName)
{
	curFile = QFileInfo(fileName).canonicalFilePath();

	setWindowTitle(tr("%1[*] - %2").arg(QTeXUtils::strippedName(curFile)).arg(tr(TEXWORKS_NAME)));

	QTeXApp *app = qobject_cast<QTeXApp*>(qApp);
	if (app)
		app->updateWindowMenus();
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

void PDFDocument::zoomToRight(QWidget *otherWindow)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(otherWindow == NULL ? this : otherWindow);
	screenRect.setTop(screenRect.top() + 22);
	screenRect.setLeft((screenRect.left() + screenRect.right()) / 2 + 1);
	screenRect.setBottom(screenRect.bottom() - 1);
	screenRect.setRight(screenRect.right() - 1);
	setGeometry(screenRect);
}

void PDFDocument::showPage(int page)
{
	pageLabel->setText(tr("page %1 of %2").arg(page).arg(document->numPages()));
}

void PDFDocument::showScale(double scale)
{
	scaleLabel->setText(tr("%1%").arg(round(scale * 10000.0) / 100.0));
}

void PDFDocument::retypeset()
{
	if (sourceDoc != NULL)
		sourceDoc->typeset();
}

void PDFDocument::goToSource()
{
	if (sourceDoc != NULL)
		sourceDoc->selectWindow();
}

void PDFDocument::enablePageActions(int pageIndex)
{
// disabling these leads to a crash if we hit the end of document while auto-repeating a key
// (seems like a Qt bug, but needs further investigation)
//	actionFirst_Page->setEnabled(pageIndex > 0);
//	actionPrevious_Page->setEnabled(pageIndex > 0);
//	actionNext_Page->setEnabled(pageIndex < document->numPages() - 1);
//	actionLast_Page->setEnabled(pageIndex < document->numPages() - 1);
}

void PDFDocument::enableZoomActions(double scaleFactor)
{
	actionZoom_In->setEnabled(scaleFactor < kMaxScaleFactor);
	actionZoom_Out->setEnabled(scaleFactor > kMinScaleFactor);
}

void PDFDocument::adjustScaleActions(autoScaleOption scaleOption)
{
	actionFit_to_Window->setChecked(scaleOption == kFitWindow);
	actionFit_to_Width->setChecked(scaleOption == kFitWidth);
}

void PDFDocument::toggleFullScreen()
{
	if (windowState() & Qt::WindowFullScreen) {
		// exiting full-screen mode
		statusBar()->show();
		toolBar->show();
		showNormal();
		pdfWidget->restoreState();
		actionFull_Screen->setChecked(false);
	}
	else {
		// entering full-screen mode
		statusBar()->hide();
		toolBar->hide();
		showFullScreen();
		pdfWidget->saveState();
		pdfWidget->fitWindow(true);
		actionFull_Screen->setChecked(true);
	}
}

void PDFDocument::resetMagnifier()
{
	pdfWidget->resetMagnifier();
}

void PDFDocument::setResolution(int res)
{
	if (res > 0)
		pdfWidget->setResolution(res);
}

void PDFDocument::enableTypesetAction(bool enabled)
{
	actionTypeset->setEnabled(enabled);
}
