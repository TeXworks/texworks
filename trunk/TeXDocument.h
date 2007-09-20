#ifndef TeXDocument_H
#define TeXDocument_H

#include <QMainWindow>
#include <QList>

#include "ui_TeXDocument.h"

class QAction;
class QMenu;
class QTextEdit;
class QToolBar;
class QLabel;

class TeXHighlighter;
class PDFDocument;

class TeXDocument : public QMainWindow, private Ui::TeXDocument
{
	Q_OBJECT

public:
	TeXDocument();
	TeXDocument(const QString &fileName);

	virtual ~TeXDocument();

	static TeXDocument *findDocument(const QString &fileName);
	static QList<TeXDocument*> documentList()
		{
			return docList;
		}


	void open(const QString &fileName);
	bool untitled()
		{ return isUntitled; }
	QString fileName() const
		{ return curFile; }

protected:
	void closeEvent(QCloseEvent *event);

public slots:
	void doFindAgain();
	void selectWindow();
	
private slots:
	void newFile();
	void open();
	bool save();
	bool saveAs();
	void doFontDialog();
	void doLineDialog();
	void doFindDialog();
	void doReplaceDialog();
	void doIndent();
	void doUnindent();
	void doComment();
	void doUncomment();
	void copyToFind();
	void copyToReplace();
	void findSelection();
	void pdfClosed();
	void updateRecentFileActions();
	void updateWindowMenu();
	void showCursorPosition();

private:
	void init();
	bool maybeSave();
	void loadFile(const QString &fileName);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void prefixLines(const QString &prefix);
	void unPrefixLines(const QString &prefix);
	void zoomToLeft();

	QString curFile;
	bool isUntitled;
	TeXHighlighter *highlighter;
	PDFDocument *pdfDoc;

	QLabel *lineNumberLabel;
	int statusLine;
	int statusTotal;

	QList<QAction*> recentFileActions;
	QMenu *menuRecent;

	static QList<TeXDocument*> docList;
};

#endif
