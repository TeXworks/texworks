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

#ifndef COMPLETING_EDIT_H
#define COMPLETING_EDIT_H

#include <QTextEdit>
#include <QTimer>

#include <hunspell.h>

class QCompleter;
class QStandardItemModel;
class QTextCodec;

class CompletingEdit : public QTextEdit
{
	Q_OBJECT

public:
	CompletingEdit(QWidget *parent = 0);
	~CompletingEdit();

	void setSpellChecker(Hunhandle *h, QTextCodec *codec);

	bool selectWord(QTextCursor& cursor);

	void setAutoIndentMode(int index);

	static QStringList autoIndentModes();

signals:
	void syncClick(int);

protected:
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void focusInEvent(QFocusEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mouseDoubleClickEvent(QMouseEvent *e);
	virtual void contextMenuEvent(QContextMenuEvent *e);
	virtual void dragEnterEvent(QDragEnterEvent *e);
	virtual void dropEvent(QDropEvent *e);
	virtual void timerEvent(QTimerEvent *e);
	virtual bool canInsertFromMimeData(const QMimeData *source) const;
	virtual void insertFromMimeData(const QMimeData *source);

private slots:
	void cursorPositionChangedSlot();
	void correction(const QString& suggestion);
	void resetExtraSelections();
	void jumpToPdf();

private:
	void setCompleter(QCompleter *c);

	void showCompletion(const QString& completion, int insOffset = -1);
	void showCurrentCompletion();

	void loadCompletionsFromFile(QStandardItemModel *model, const QString& filename);
	void loadCompletionFiles(QCompleter *theCompleter);

	void handleCompletionShortcut(QKeyEvent *e);
	void handleReturn(QKeyEvent *e);
	void handleBackspace(QKeyEvent *e);
	void handleOtherKey(QKeyEvent *e);

	QTextCursor wordSelectionForPos(const QPoint& pos);
	QTextCursor blockSelectionForPos(const QPoint& pos);
	
	enum MouseMode {
		none,
		ignoring,
		synctexClick,
		normalSelection,
		extendingSelection,
		dragSelecting
	};
	MouseMode mouseMode;
	
	QTextCursor dragStartCursor;

	int droppedOffset, droppedLength;
	
	QBasicTimer clickTimer;
	QPoint clickPos;
	int clickCount;
	
	static void loadIndentModes();

	struct IndentMode {
		QString	name;
		QRegExp	regex;
	};
	static QList<IndentMode> *indentModes;
	int autoIndentMode;

	int prefixLength;

	QCompleter *c;
	QTextCursor cmpCursor;

	QString prevCompletion; // used with multiple entries for the same key (e.g., "--")
	int itemIndex;
	int prevRow;

	QTextCursor currentWord;
	Hunhandle *pHunspell;
	QTextCodec *spellingCodec;

	QTextCursor	currentCompletionRange;

	static QTextCharFormat	*currentCompletionFormat;
	static QTextCharFormat	*braceMatchingFormat;
	
	static QCompleter	*sharedCompleter;
};

#endif // COMPLETING_EDIT_H
