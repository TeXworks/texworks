#ifndef TeXDocument_H
#define TeXDocument_H

#include <QMainWindow>

#include "ui_TeXDocument.h"

class QAction;
class QMenu;
class QTextEdit;
class QToolBar;

class TeXHighlighter;

class TeXDocument : public QMainWindow, private Ui::TeXDocument
{
	Q_OBJECT

public:
	TeXDocument();
	TeXDocument(const QString &fileName);
	virtual ~TeXDocument();

	static TeXDocument *findDocument(const QString &fileName);
	static QString strippedName(const QString &fullFileName);

	void open(const QString &fileName);
	bool untitled()
		{ return isUntitled; }

protected:
	void closeEvent(QCloseEvent *event);

public slots:
	void doFindAgain();
	
private slots:
	void newFile();
	void open();
	bool save();
	bool saveAs();
	void doFontDialog();
	void doLineDialog();
	void doFindDialog();
	void doIndent();
	void doUnindent();
	void doComment();
	void doUncomment();
	void copyToFind();
	void copyToReplace();
	void findSelection();

private:
	void init();
	bool maybeSave();
	void loadFile(const QString &fileName);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void prefixLines(const QString &prefix);
	void unPrefixLines(const QString &prefix);
	void updateRecentFileActions();

	QString curFile;
	bool isUntitled;
	TeXHighlighter *highlighter;
};

#endif
