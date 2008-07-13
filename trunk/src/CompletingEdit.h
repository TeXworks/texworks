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
	see <http://tug.org/texworks/>.
*/

#ifndef COMPLETING_EDIT_H
#define COMPLETING_EDIT_H

#include <QTextEdit>

#include <hunspell/hunspell.h>

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

signals:
	void syncClick(int);

protected:
	virtual void keyPressEvent(QKeyEvent *e);
	virtual void focusInEvent(QFocusEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void mouseDoubleClickEvent(QMouseEvent *e);
	virtual void contextMenuEvent(QContextMenuEvent *e);

private slots:
	void clearCompleter();
	void correction(const QString& suggestion);

private:
	void setCompleter(QCompleter *c);

	void showCompletion(const QString& completion, int insOffset = -1);
	void showCurrentCompletion();

	void loadCompletionsFromFile(QStandardItemModel *model, const QString& filename);
	void loadCompletionFiles(QCompleter *theCompleter);

	QCompleter *c;
	QTextCursor cmpCursor;

	QString prevCompletion; // used with multiple entries for the same key (e.g., "--")
	int itemIndex;
	int prevRow;

	QTextCursor currentWord;
	Hunhandle *pHunspell;
	QTextCodec *spellingCodec;

	static QCompleter	*sharedCompleter;
};

#endif // COMPLETING_EDIT_H
