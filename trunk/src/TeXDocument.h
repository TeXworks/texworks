/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-08  Jonathan Kew

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

	For links to further information, or to contact the author,
	see <http://texworks.org/>.
*/

#ifndef TeXDocument_H
#define TeXDocument_H

#include <QMainWindow>
#include <QList>
#include <QRegExp>
#include <QProcess>
#include <QDateTime>

#include "ui_TeXDocument.h"

#include "FindDialog.h"

#include <hunspell.h>

class QAction;
class QMenu;
class QTextEdit;
class QToolBar;
class QLabel;
class QComboBox;
class QActionGroup;
class QTextCodec;
class QFileSystemWatcher;

class TeXHighlighter;
class PDFDocument;

class TeXDocument : public QMainWindow, private Ui::TeXDocument
{
	Q_OBJECT

public:
	TeXDocument();
	TeXDocument(const QString &fileName, bool asTemplate = false);

	virtual ~TeXDocument();

	static TeXDocument *findDocument(const QString &fileName);
	static QList<TeXDocument*> documentList()
		{
			return docList;
		}
	static TeXDocument *openDocument(const QString &fileName, bool activate = true, bool raiseWindow = true,
									 int lineNo = 0, int selStart = -1, int selEnd = -1);

	TeXDocument *open(const QString &fileName);
	void makeUntitled();
	bool untitled()
		{ return isUntitled; }
	QString fileName() const
		{ return curFile; }
	QTextCursor textCursor()
		{ return textEdit->textCursor(); }
	QTextDocument* textDoc()
		{ return textEdit->document(); }
	QString getLineText(int lineNo) const;
	CompletingEdit* editor()
		{ return textEdit; }

	PDFDocument* pdfDocument()
		{ return pdfDoc; }

	void addTag(const QTextCursor& cursor, int level, const QString& text);
	int removeTags(int offset, int len);
	void goToTag(int index);
	void tagsChanged();

	class Tag {
	public:
		QTextCursor	cursor;
		int			level;
		QString		text;
		Tag(const QTextCursor& curs, int lvl, const QString& txt)
			: cursor(curs), level(lvl), text(txt) { };
	};
	const QList<Tag> getTags() const
		{ return tags; }

signals:
	void syncFromSource(const QString&, int);
	void activatedWindow(QWidget*);
	void tagListUpdated();

protected:
	virtual void changeEvent(QEvent *event);
	virtual void closeEvent(QCloseEvent *event);
	virtual bool event(QEvent *event);
	virtual void dragEnterEvent(QDragEnterEvent *event);
	virtual void dragMoveEvent(QDragMoveEvent *event);
	virtual void dragLeaveEvent(QDragLeaveEvent *event);
	virtual void dropEvent(QDropEvent *event);

public slots:
	void selectWindow(bool activate = true);
	void typeset();
	void interrupt();
	
private slots:
	void newFile();
	void newFromTemplate();
	void open();
	bool save();
	bool saveAs();
	void revert();
	void maybeEnableSaveAndRevert(bool modified);
	void clear();
	void clipboardChanged();
	void doFontDialog();
	void doLineDialog();
	void doFindDialog();
	void doFindAgain(bool fromDialog = false);
	void doReplaceDialog();
	void doReplaceAgain();
	void doReplace(ReplaceDialog::DialogCode mode);
	void doIndent();
	void doUnindent();
	void doComment();
	void doUncomment();
	void toUppercase();
	void toLowercase();
	void toggleCase();
	void balanceDelimiters();
	void doHardWrapDialog();
	void setLineNumbers(bool displayNumbers);
	void setWrapLines(bool wrap);
	void setSyntaxColoring(int index);
	void copyToFind();
	void copyToReplace();
	void findSelection();
	void showSelection();
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
	void acceptInputLine();
	void goToPreview();
	void syncClick(int lineNo);
	void openAt(QAction *action);
	void selectedEngine(QAction* engineAction);
	void selectedEngine(const QString& name);
	void contentsChanged(int position, int charsRemoved, int charsAdded);
	void setLanguage(const QString& lang);
	void hideFloatersUnlessThis(QWidget* currWindow);
	void sideBySide();
	void placeOnLeft();
	void placeOnRight();
	void reloadIfChangedOnDisk();
	void removeAuxFiles();
	void setupFileWatcher();

private:
	void init();
	bool maybeSave();
	void detachPdf();
	bool saveFilesHavingRoot(const QString& aRootFile);
	void clearFileWatcher();
	QTextCodec *scanForEncoding(const QString &peekStr, bool &hasMetadata, QString &reqName);
	QString readFile(const QString &fileName, QTextCodec **codecUsed);
	void loadFile(const QString &fileName, bool asTemplate = false, bool inBackground = false);
	bool saveFile(const QString &fileName);
	void setCurrentFile(const QString &fileName);
	bool getPreviewFileName(QString &pdfName);
	bool showPdfIfAvailable();
	void prefixLines(const QString &prefix);
	void unPrefixLines(const QString &prefix);
	void replaceSelection(const QString& newText);
	void doHardWrap(int lineWidth, bool rewrap);
	void zoomToLeft(QWidget *otherWindow);
	QTextCursor doSearch(QTextDocument *theDoc, const QString& searchText, const QRegExp *regex,
						 QTextDocument::FindFlags flags, int rangeStart, int rangeEnd);
	int doReplaceAll(const QString& searchText, QRegExp* regex, const QString& replacement,
						QTextDocument::FindFlags flags, int rangeStart = -1, int rangeEnd = -1);
	void showConsole();
	void hideConsole();
	void goToLine(int lineNo, int selStart = -1, int selEnd = -1);
	void updateTypesettingAction();
	void findRootFilePath();
	const QString& getRootFilePath();
	void maybeCenterSelection(int oldScrollValue = -1);
	void showFloaters();
	void presentResults(const QList<SearchResult>& results);

	TeXHighlighter *highlighter;
	PDFDocument *pdfDoc;

	QTextCodec *codec;
	QString curFile;
	QString rootFilePath;
	bool isUntitled;
	QDateTime lastModified;

	QLabel *lineNumberLabel;

	QActionGroup *engineActions;
	QString engineName;

	QComboBox *engine;
	QProcess *process;
	bool consoleWasHidden;
	bool showPdfWhenFinished;
	bool userInterrupt;
	QDateTime oldPdfTime;

	QList<QAction*> recentFileActions;
	QMenu *menuRecent;

	Hunhandle *pHunspell;

	QList<QWidget*> latentVisibleWidgets;

	QFileSystemWatcher *watcher;
	
	QList<Tag>	tags;
	bool deferTagListChanges;
	bool tagListChanged;

	QTextCursor	dragSavedCursor;

	static QList<TeXDocument*> docList;
};

#endif
