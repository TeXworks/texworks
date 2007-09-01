#ifndef TeXDocument_H
#define TeXDocument_H

#include <QMainWindow>

#include "ui_TeXDocument.h"

class QAction;
class QMenu;
class QTextEdit;
class QToolBar;

class TeXHighlighter;

const int kMaxRecentFiles = 10;

class TeXDocument : public QMainWindow, private Ui::TeXDocument
{
	Q_OBJECT

public:
	TeXDocument();
	TeXDocument(const QString &fileName);

	static TeXDocument *TeXDocument::findDocument(const QString &fileName);

protected:
	void closeEvent(QCloseEvent *event);

private slots:
	void newFile();
	void open();
	void openRecentFile();
	bool save();
	bool saveAs();
	void doFontDialog();
	void doLineDialog();
	void doFindDialog();
	void doIndent();
	void doUnindent();
	void doComment();
	void doUncomment();

private:
	void init();
	bool maybeSave();
	void open(const QString &fileName);
	void loadFile(const QString &fileName);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	QString strippedName(const QString &fullFileName);
	void prefixLines(const QString &prefix);
	void unPrefixLines(const QString &prefix);
	void updateRecentFileActions();

	QString curFile;
	bool isUntitled;
	TeXHighlighter *highlighter;

	QMenu *menuRecent;
	QAction *recentFileActs[kMaxRecentFiles];
};

#endif
