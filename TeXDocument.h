#ifndef TeXDocument_H
#define TeXDocument_H

#include <QMainWindow>
#include <QList>
#include <QRegExp>
#include <QProcess>

#include "ui_TeXDocument.h"

#include "FindDialog.h"

class QAction;
class QMenu;
class QTextEdit;
class QToolBar;
class QLabel;
class QComboBox;

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
	static void openDocument(const QString &fileName, int lineNo = 0);


	TeXDocument *open(const QString &fileName);
	bool untitled()
		{ return isUntitled; }
	QString fileName() const
		{ return curFile; }
	QTextCursor textCursor()
		{ return textEdit->textCursor(); }

signals:
	void syncFromSource(const QString&, int);

protected:
	void closeEvent(QCloseEvent *event);
	bool event(QEvent *event);

public slots:
	void selectWindow();
	void typeset();
	void interrupt();
	
private slots:
	void newFile();
	void open();
	bool save();
	bool saveAs();
	void clear();
	void clipboardChanged();
	void doFontDialog();
	void doLineDialog();
	void doFindDialog();
	void doFindAgain();
	void doReplaceDialog();
	void doReplace(ReplaceDialog::DialogCode mode);
	void doIndent();
	void doUnindent();
	void doComment();
	void doUncomment();
	void setWrapLines(bool wrap);
	void copyToFind();
	void copyToReplace();
	void findSelection();
	void pdfClosed();
	void updateRecentFileActions();
	void updateWindowMenu();
	void updateEngineList();
	void showCursorPosition();
	void editMenuAboutToShow();
	void processStandardOutput();
	void processError(QProcess::ProcessError error);
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus);
	void toggleConsoleVisibility();
	void goToPreview();
	void syncClick(int lineNo);
	void openAt(QAction *action);

private:
	void init();
	bool maybeSave();
	void loadFile(const QString &fileName);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	void showPdfIfAvailable();
	void prefixLines(const QString &prefix);
	void unPrefixLines(const QString &prefix);
	void zoomToLeft(QWidget *otherWindow);
	QTextCursor doSearch(const QString& searchText, const QRegExp *regex, QTextDocument::FindFlags flags, int rangeStart, int rangeEnd);
	void showConsole();
	void hideConsole();
	void goToLine(int lineNo);
	void updateTypesettingAction();

	QString curFile;
	bool isUntitled;
	TeXHighlighter *highlighter;
	PDFDocument *pdfDoc;

	QLabel *lineNumberLabel;
	int statusLine;
	int statusTotal;

	QComboBox *engine;
	QProcess *process;

	QList<QAction*> recentFileActions;
	QMenu *menuRecent;

	static QList<TeXDocument*> docList;
};

#endif
