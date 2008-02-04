#ifndef PDFDocument_H
#define PDFDocument_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>
#include <QList>

#include "poppler-qt4.h"
#include "GlobalParams.h"

#include "ui_PDFDocument.h"

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

signals:
	void changedPage(int);
	void changedZoom(double);
	void changedScaleOption(autoScaleOption);

protected:
	virtual void paintEvent(QPaintEvent *event);

	virtual void mousePressEvent(QMouseEvent *event);
	virtual void mouseReleaseEvent(QMouseEvent *event);
	virtual void mouseMoveEvent(QMouseEvent *event);

private:
	void init();
	void reloadPage();
	void adjustSize();
	void updateStatusBar();
	
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
	bool magnifying;
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

signals:
	void windowResized();

private:
	void init();
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);

	QString curFile;
	
	Poppler::Document	*document;
	
	PDFWidget	*pdfWidget;
	QScrollArea	*scrollArea;

	TeXDocument *sourceDoc;

	QLabel *pageLabel;
	QLabel *scaleLabel;
	QList<QAction*> recentFileActions;
	QMenu *menuRecent;

	static QList<PDFDocument*> docList;
};

#endif
