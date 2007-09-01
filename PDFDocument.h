#ifndef PDFDocument_H
#define PDFDocument_H

#include <QMainWindow>
#include <QImage>
#include <QLabel>

#include "poppler/qt4/poppler-qt4.h"

#include "ui_PDFDocument.h"

class QAction;
class QMenu;
class QToolBar;
class QScrollArea;


class PDFWidget : public QLabel
{
	Q_OBJECT

public:
	PDFWidget();
	void setDocument(Poppler::Document *doc);

private slots:
	void goFirst();
	void goPrev();
	void goNext();
	void goLast();
	void doPageDialog();
	
	void actualSize();
	void fitWidth();
	void fitWindow();
	void zoomIn();
	void zoomOut();

protected:
	virtual void paintEvent(QPaintEvent *event);

private:
	void init();
	void reloadPage();
	void adjustSize();
	void updateStatusBar();
	
	Poppler::Document	*document;
	Poppler::Page		*page;

	int pageIndex;
	double	scaleFactor;

	int prevPage;
	QRect	prevGeom;
	
	QImage	image;
};


class PDFDocument : public QMainWindow, private Ui::PDFDocument
{
	Q_OBJECT

public:
	PDFDocument(const QString &fileName);

	static PDFDocument *PDFDocument::findDocument(const QString &fileName);

protected:
	void closeEvent(QCloseEvent *event);

private slots:

private:
	void init();
	void loadFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	QString strippedName(const QString &fullFileName);

	QString curFile;
	QImage	image;
	
	Poppler::Document	*document;
	
	PDFWidget	*pdfWidget;
	QScrollArea	*scrollArea;
};

#endif
