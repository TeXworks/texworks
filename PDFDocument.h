#ifndef PDFDocument_H
#define PDFDocument_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QList>
#include <QCursor>
#include <QButtonGroup>

#include "poppler-qt4.h"

#include "ui_PDFDocument.h"

const int kDefault_MagnifierSize = 2;
const bool kDefault_CircularMagnifier = true;

class QAction;
class QMenu;
class QToolBar;
class QScrollArea;
class TeXDocument;

class PDFMagnifier : public QLabel
{
	Q_OBJECT

public:
	PDFMagnifier(QWidget *parent, double inDpi);
	void setPage(Poppler::Page *p, double scale);

protected:
	virtual void paintEvent(QPaintEvent *event);
	virtual void resizeEvent(QResizeEvent *event);

private:
	Poppler::Page	*page;
	double	scaleFactor;
	double	dpi;
	QImage	image;
	
	QPoint	imageLoc;
	QSize	imageSize;
	double	imageDpi;
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
	void setDocument(Poppler::Document *doc);

	void saveState(); // used when toggling full screen mode
	void restoreState();
	void setResolution(int res);
	void resetMagnifier();
	void goToPage(int pageIndex);
	void setHighlightBoxes(const QList<QRectF>& boxlist);

private slots:
	void goFirst();
	void goPrev();
	void goNext();
	void goLast();
	void doPageDialog();
	
	void actualSize();
	void fitWidth(bool checked);
	void zoomIn();
	void zoomOut();
	
public slots:
	void windowResized();
	void fitWindow(bool checked);
	void setTool(int tool);

signals:
	void changedPage(int);
	void changedZoom(double);
	void changedScaleOption(autoScaleOption);
	void syncClick(int, double, const QPoint&);

protected:
	virtual void paintEvent(QPaintEvent *event);

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseDoubleClickEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

	virtual void keyPressEvent(QKeyEvent *event);
	virtual void keyReleaseEvent(QKeyEvent *event);

	virtual void focusInEvent(QFocusEvent *event);

private:
	void init();
	void reloadPage();
	void adjustSize();
	void updateStatusBar();
	void updateCursor();
	void useMagnifier(const QMouseEvent *inEvent);
	
	Poppler::Document	*document;
	Poppler::Page		*page;

	int pageIndex;
	double	scaleFactor;
	double	dpi;
	autoScaleOption scaleOption;

	double			saveScaleFactor;
	autoScaleOption	saveScaleOption;
	
	QImage	image;
	QRect	imageRect;
	double	imageDpi;
	Poppler::Page	*imagePage;

	PDFMagnifier	*magnifier;
	int		currentTool;	// the current tool selected in the toolbar
	int		usingTool;	// the tool actually being used in an ongoing mouse drag

	QList<QRectF>	highlightBoxes;

	static QCursor	*magnifierCursor;
	static QCursor	*zoomInCursor;
	static QCursor	*zoomOutCursor;
};


class PDFDocument : public QMainWindow, private Ui::PDFDocument
{
	Q_OBJECT

public:
	PDFDocument(const QString &fileName, TeXDocument *sourceDoc);
	virtual ~PDFDocument();

	static PDFDocument *findDocument(const QString &fileName);
	static QList<PDFDocument*> documentList()
		{
			return docList;
		}

	QString fileName() const
		{ return curFile; }

	void zoomToRight(QWidget *otherWindow);
	void reload();
	void showScale(double scale);
	void showPage(int page);
	void setResolution(int res);
	void resetMagnifier();
	void enableTypesetAction(bool enabled);

protected:
	virtual void resizeEvent(QResizeEvent *event);

public slots:
	void selectWindow();

private slots:
	void updateRecentFileActions();
	void updateWindowMenu();
	void enablePageActions(int);
	void enableZoomActions(double);
	void adjustScaleActions(autoScaleOption);
	void retypeset();
	void goToSource();
	void toggleFullScreen();
	void syncClick(int page, double scaleFactor, const QPoint& pos);
	void syncFromSource(const QString& sourceFile, int lineNo);

signals:
	void windowResized();

private:
	void init();
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void loadSyncData();

	QString curFile;
	
	Poppler::Document	*document;
	
	PDFWidget	*pdfWidget;
	QScrollArea	*scrollArea;
	QButtonGroup	*toolButtonGroup;

	TeXDocument *sourceDoc;

	QLabel *pageLabel;
	QLabel *scaleLabel;
	QList<QAction*> recentFileActions;
	QMenu *menuRecent;

	typedef struct HBox {
		int tag;
		int line;
		int x;
		int y;
		int w;
		int h;
		int first;
		int last;
	} HBox;
	typedef QVector<HBox> PageSyncInfo;

	QList<PageSyncInfo> pageSyncInfo;
	QHash<int,QString> tagToFile;

	static QList<PDFDocument*> docList;
};

#endif
