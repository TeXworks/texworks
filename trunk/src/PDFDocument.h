/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2013  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/

#ifndef PDFDocument_H
#define PDFDocument_H

#include "TWScriptable.h"

#include <QImage>
#include <QLabel>
#include <QList>
#include <QCursor>
#include <QButtonGroup>
#include <QPainterPath>
#include <QTimer>
#include <QMouseEvent>

#include "TWApp.h"
#include "FindDialog.h"
#if QT_VERSION < 0x050000
#include "poppler-qt4.h"
#else
#include "poppler-qt5.h"
#endif
#include "TWSynchronizer.h"

#include "ui_PDFDocument.h"

const int kDefault_MagnifierSize = 2;
const bool kDefault_CircularMagnifier = true;
const int kDefault_PreviewScaleOption = 1;
const int kDefault_PreviewScale = 200;

const int kPDFWindowStateVersion = 1;

class QAction;
class QMenu;
class QToolBar;
class QScrollArea;
class TeXDocument;
class QShortcut;
class QFileSystemWatcher;

class PDFMagnifier : public QLabel
{
	Q_OBJECT

public:
	PDFMagnifier(QWidget *parent, qreal inDpi);
	void setPage(Poppler::Page *p, qreal scale);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	Poppler::Page	*page;
	qreal	scaleFactor;
	qreal	parentDpi;
	QImage	image;
	
	QPoint	imageLoc;
	QSize	imageSize;
	qreal	imageDpi;
	Poppler::Page	*imagePage;
};

typedef enum {
	kFixedMag,
	kFitWidth,
	kFitWindow
} autoScaleOption;

class PDFWidget : public QLabel
{
	Q_OBJECT

public:
	PDFWidget();
	virtual ~PDFWidget();
	
	void setDocument(Poppler::Document *doc);

	void saveState(); // used when toggling full screen mode
	void restoreState();
	void setResolution(int res);
	void resetMagnifier();
	void goToPage(int pageIndex);
	void setHighlightPath(const QPainterPath& path);
	void goToDestination(const QString& destName);
	int getCurrentPageIndex() { return pageIndex; }
	void reloadPage();
	void updateStatusBar();
	QString selectedText(const QList<QPolygonF> & selection, QMap<int, QRectF> * wordBoxes = NULL, QMap<int, QRectF> * charBoxes = NULL);

private slots:
	void goFirst();
	void goPrev();
	void goNext();
	void goLast();
	void doPageDialog();
	
	void zoomIn();
	void zoomOut();
	void jumpToSource();
	
	void upOrPrev();
	void leftOrPrev();
	void downOrNext();
	void rightOrNext();

	void clearHighlight();
	
public slots:
	void windowResized();
	void fixedScale(qreal scale = 1.0);
	void fitWidth(bool checked = true);
	void fitWindow(bool checked = true);
	void setTool(int tool);

signals:
	void changedPage(int);
	void changedZoom(qreal);
	void changedScaleOption(autoScaleOption);
	void syncClick(int, const QPointF&);

protected:
	virtual void paintEvent(QPaintEvent *event);

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);

	virtual void focusInEvent(QFocusEvent *event);

	virtual void contextMenuEvent(QContextMenuEvent *event);
	virtual void wheelEvent(QWheelEvent *event);

private:
	void init();
	void adjustSize();
	void updateCursor();
	void updateCursor(const QPoint& pos);
	void useMagnifier(const QMouseEvent *inEvent);
	void goToDestination(const Poppler::LinkDestination& dest);
	void doLink(const Poppler::Link *link);
	void doZoom(const QPoint& clickPos, int dir);
	QScrollArea* getScrollArea();
	
	Poppler::Document	*document;
	Poppler::Page		*page;
	Poppler::Link		*clickedLink;

	int pageIndex;
	qreal	scaleFactor;
	qreal	dpi;
	autoScaleOption scaleOption;

	qreal			saveScaleFactor;
	autoScaleOption	saveScaleOption;

	QAction	*ctxZoomInAction;
	QAction	*ctxZoomOutAction;
	QShortcut *shortcutUp;
	QShortcut *shortcutLeft;
	QShortcut *shortcutDown;
	QShortcut *shortcutRight;
	
	QImage	image;
	QRect	imageRect;
	qreal	imageDpi;
	Poppler::Page	*imagePage;

	PDFMagnifier	*magnifier;
	int		currentTool;	// the current tool selected in the toolbar
	int		usingTool;	// the tool actually being used in an ongoing mouse drag

	QPainterPath	highlightPath;
	QTimer highlightRemover;
	
	static QCursor	*magnifierCursor;
	static QCursor	*zoomInCursor;
	static QCursor	*zoomOutCursor;
	static QCursor	*synctexCursor;
};


class PDFDocument : public TWScriptable, private Ui::PDFDocument
{
	Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName)

public:
	PDFDocument(const QString &fileName, TeXDocument *sourceDoc = NULL);
	virtual ~PDFDocument();

	static PDFDocument *findDocument(const QString &fileName);
	static QList<PDFDocument*> documentList()
		{
			return docList;
		}

	QString fileName() const
		{ return curFile; }

	void zoomToRight(QWidget *otherWindow);
	void showScale(qreal scale);
	void showPage(int page);
	void setResolution(int res);
	void resetMagnifier();
	void enableTypesetAction(bool enabled);
	void updateTypesettingAction(bool processRunning);
	void goToDestination(const QString& destName);
	void linkToSource(TeXDocument *texDoc);
	bool hasSyncData() const { return _synchronizer != NULL; }

	Poppler::Document *popplerDoc()
		{
			return document;
		}
	
	PDFWidget *widget()
		{
			return pdfWidget;
		}

protected:
	virtual void changeEvent(QEvent *event);
	virtual bool event(QEvent *event);
	virtual void closeEvent(QCloseEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dropEvent(QDropEvent *event);

public slots:
	void texActivated(TeXDocument * texDoc);
	void texClosed(QObject *obj);
	void reload();
	void retypeset();
	void interrupt();
	void sideBySide();
	void doFindDialog();
	void doFindAgain(bool newSearch = false);
	void goToSource();
	void toggleFullScreen();
	void syncFromSource(const QString& sourceFile, int lineNo, int col, bool activatePreview);
	void print();
	
private slots:
	void updateRecentFileActions();
	void clearRecentFiles() { TWApp::instance()->clearRecentFiles(); }
	void updateWindowMenu();
	void enablePageActions(int);
	void enableZoomActions(qreal);
	void adjustScaleActions(autoScaleOption);
	void syncClick(int page, const QPointF& pos);
	void reloadWhenIdle();
	void scaleLabelClick(QMouseEvent * event) { showScaleContextMenu(event->pos()); }
	void showScaleContextMenu(const QPoint pos);
	void setScaleFromContextMenu(const QString & strZoom);

signals:
	void reloaded();
	void activatedWindow(QWidget*);

private:
	void init();
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void loadSyncData();
	void saveRecentFileInfo();

	QString curFile;
	
	Poppler::Document	*document;
	
	PDFWidget	*pdfWidget;
	QScrollArea	*scrollArea;
	QButtonGroup	*toolButtonGroup;

	QLinkedList<TeXDocument*> sourceDocList;

	QLabel *pageLabel;
	QLabel *scaleLabel;
	QList<QAction*> recentFileActions;
	QShortcut *exitFullscreen;

	QFileSystemWatcher *watcher;
	QTimer *reloadTimer;
	
	bool openedManually;
	
	static QList<PDFDocument*> docList;

	TWSyncTeXSynchronizer * _synchronizer;
	
	PDFSearchResult lastSearchResult;
	// stores the page idx a search was started on
	// after wrapping the search will continue only up to this page
	int firstSearchPage; 
};

#endif
