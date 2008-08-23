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

#include "CompletingEdit.h"
#include "TWUtils.h"

#include <QCompleter>
#include <QKeyEvent>
#include <QAbstractItemView>
#include <QApplication>
#include <QModelIndex>
#include <QStandardItem>
#include <QStandardItemModel>
#include <QTextCursor>
#include <QFileInfo>
#include <QFile>
#include <QDir>
#include <QMenu>
#include <QTextStream>
#include <QTextCodec>
#include <QAbstractTextDocumentLayout>
#include <QSignalMapper>
#include <QTextDocument>
#include <QTextBlock>
#include <QScrollBar>
#include <QTimer>


CompletingEdit::CompletingEdit(QWidget *parent)
	: QTextEdit(parent), c(NULL), cmpCursor(QTextCursor()), pHunspell(NULL), spellingCodec(NULL)
{
	if (sharedCompleter == NULL) {
		sharedCompleter = new QCompleter(qApp);
		sharedCompleter->setCompletionMode(QCompleter::InlineCompletion);
		sharedCompleter->setCaseSensitivity(Qt::CaseInsensitive);
		loadCompletionFiles(sharedCompleter);
	}
}

CompletingEdit::~CompletingEdit()
{
	setCompleter(NULL);
}

void CompletingEdit::setCompleter(QCompleter *completer)
{
	c = completer;
	if (!c)
		return;

	c->setWidget(this);
}

void CompletingEdit::clearCompleter()
{
	setCompleter(NULL);
	setExtraSelections(QList<ExtraSelection>());
	disconnect(this, SIGNAL(cursorPositionChanged()), this, SLOT(clearCompleter()));
}

void CompletingEdit::mousePressEvent(QMouseEvent *e)
{
	if (e->modifiers() == Qt::ControlModifier)
		e->accept();
	else
		QTextEdit::mousePressEvent(e);
}

void CompletingEdit::mouseReleaseEvent(QMouseEvent *e)
{
	if (e->modifiers() == Qt::ControlModifier) {
		e->accept();
		QTextCursor cursor = cursorForPosition(e->pos());
		emit syncClick(cursor.blockNumber() + 1);
	}
	else
		QTextEdit::mouseReleaseEvent(e);
}

bool CompletingEdit::selectWord(QTextCursor& cursor)
{
	if (cursor.selectionEnd() - cursor.selectionStart() > 1)
		return false;	// actually an error by the caller

	const QTextBlock block = document()->findBlock(cursor.selectionStart());
	const QString text = block.text();
	if (text.length() < 1) // empty line
		return false;

	int start, end;
	bool result = TWUtils::findNextWord(text, cursor.selectionStart() - block.position(), start, end);
	cursor.setPosition(block.position() + start);
	cursor.setPosition(block.position() + end, QTextCursor::KeepAnchor);

	return result;
}

void CompletingEdit::mouseDoubleClickEvent(QMouseEvent *e)
{
	if (e->modifiers() == Qt::ControlModifier)
		e->accept();
	else if (e->modifiers() != Qt::NoModifier)
		QTextEdit::mouseDoubleClickEvent(e);
	else {
		// don't like QTextEdit's selection behavior, so try to improve it here
		QPoint	pos = e->pos() + QPoint(horizontalScrollBar()->value(), verticalScrollBar()->value());
		int cursorPos = document()->documentLayout()->hitTest(pos, Qt::FuzzyHit);
		if (cursorPos == -1)
			return;

		QTextCursor cursor(document());
		cursor.setPosition(cursorPos);
		cursor.setPosition(cursorPos + 1, QTextCursor::KeepAnchor);
		
		// check if click was within the char to the right of cursor; if so we select forwards
		QRect r = cursorRect(cursor);
		if (r.contains(pos)) {
			// Currently I don't seem to be getting a useful answer from cursorRect(), it's always zero-width :-(
			// and so this path will not be used, but leaving it here in hopes of fixing it some day
//			QString s = cursor.selectedText();
//			if (isPairedChar(s)) ...
			(void)selectWord(cursor);
			setTextCursor(cursor);
			e->accept();
			return;
		}

		if (cursorPos > 0) {
			cursorPos -= 1;
			cursor.setPosition(cursorPos);
			cursor.setPosition(cursorPos + 1, QTextCursor::KeepAnchor);
			// don't test because the rect will be zero width (see above)!
	//		r = cursorRect(cursor);
	//		if (r.contains(pos)) {
				const QString plainText = toPlainText();
				QChar curChr = plainText[cursorPos];
				QChar c;
				c = TWUtils::closerMatching(curChr);
				if (c != 0) {
					int balancePos = TWUtils::balanceDelim(plainText, cursorPos + 1, c, 1);
					if (balancePos < 0)
						QApplication::beep();
					else
						cursor.setPosition(balancePos + 1, QTextCursor::KeepAnchor);
					setTextCursor(cursor);
					e->accept();
					return;
				}
				c = TWUtils::openerMatching(curChr);
				if (c != 0) {
					int balancePos = TWUtils::balanceDelim(plainText, cursorPos - 1, c, -1);
					if (balancePos < 0)
						QApplication::beep();
					else {
						cursor.setPosition(balancePos);
						cursor.setPosition(cursorPos + 1, QTextCursor::KeepAnchor);
					}
					setTextCursor(cursor);
					e->accept();
					return;
				}
				(void)selectWord(cursor);
				setTextCursor(cursor);
				e->accept();
				return;
	//		}
		}

		// else fall back on whatever QTextEdit does
		QTextEdit::mouseDoubleClickEvent(e);
	}
}

void CompletingEdit::focusInEvent(QFocusEvent *e)
{
	if (c)
		c->setWidget(this);
	QTextEdit::focusInEvent(e);
}

void CompletingEdit::clearExtraSelections()
{
	setExtraSelections(QList<ExtraSelection>());
}

void CompletingEdit::keyPressEvent(QKeyEvent *e)
{
	bool isShortcut = (e->key() == Qt::Key_Escape);

	if (!isShortcut) {	// not the shortcut key, so simply accept it
		if (e->text() != "") {
			clearCompleter();
			cmpCursor = QTextCursor();
		}
		int pos = textCursor().selectionStart(); // remember cursor before the keystroke
		QTextEdit::keyPressEvent(e);
		if ((e->modifiers() & Qt::ControlModifier) == 0) { // not a command key - maybe do brace matching
			QTextCursor cursor = textCursor();
			if (!cursor.hasSelection()) {
				if (cursor.selectionStart() == pos + 1 || cursor.selectionStart() == pos - 1) {
					if (cursor.selectionStart() == pos - 1) // we moved backward, set pos to look at the char we just passed over
						--pos;
					const QString text = document()->toPlainText();
					int match = -2;
					QChar c;
					if (pos > 0 && (c = TWUtils::openerMatching(text[pos])) != 0)
						match = TWUtils::balanceDelim(text, pos - 1, c, -1);
					else if (pos < text.length() - 1 && (c = TWUtils::closerMatching(text[pos])) != 0)
						match = TWUtils::balanceDelim(text, pos + 1, c, 1);
					if (match == -1) // no matching delimiter found
						QApplication::beep();
					else if (match >= 0) {
						QTextCharFormat	format;
						format.setBackground(QBrush("orange"));
						QList<ExtraSelection> selList;
						ExtraSelection	sel;
						sel.cursor = QTextCursor(document());
						sel.cursor.setPosition(match);
						sel.cursor.setPosition(match + 1, QTextCursor::KeepAnchor);
						sel.format = format;
						selList.append(sel);
						setExtraSelections(selList);
						QTimer::singleShot(250, this, SLOT(clearExtraSelections()));
					}
				}
			}
		}
		return;
	}
	
	if ((e->modifiers() & ~Qt::ShiftModifier) == Qt::ControlModifier) {
		if (!find(QString(0x2022), (e->modifiers() & Qt::ShiftModifier)
									? QTextDocument::FindBackward : (QTextDocument::FindFlags)0))
			QApplication::beep();
		return;
	}
	
	if (c == NULL) {
		cmpCursor = textCursor();
		if (!selectWord(cmpCursor)) {
			cmpCursor.setPosition(textCursor().selectionStart() - 1);
			selectWord(cmpCursor);
		}
		// check if the word is preceded by open-brace; if so try with that included
		int start = cmpCursor.selectionStart();
		int end = cmpCursor.selectionEnd();
		if (start > 0) { // special cases: possibly look back to include brace or hyphen(s)
			if (cmpCursor.selectedText() == "-") {
				QTextCursor hyphCursor(cmpCursor);
				int hyphPos = start;
				while (hyphPos > 0) {
					hyphCursor.setPosition(hyphPos - 1);
					hyphCursor.setPosition(hyphPos, QTextCursor::KeepAnchor);
					if (hyphCursor.selectedText() != "-")
						break;
					--hyphPos;
				}
				cmpCursor.setPosition(hyphPos);
				cmpCursor.setPosition(end, QTextCursor::KeepAnchor);
			}
			else if (cmpCursor.selectedText() != "{") {
				QTextCursor braceCursor(cmpCursor);
				braceCursor.setPosition(start - 1);
				braceCursor.setPosition(start, QTextCursor::KeepAnchor);
				if (braceCursor.selectedText() == "{") {
					cmpCursor.setPosition(start - 1);
					cmpCursor.setPosition(end, QTextCursor::KeepAnchor);
				}
			}			
		}
		
		while (1) {
			QString completionPrefix = cmpCursor.selectedText();
			if (completionPrefix != "") {
				setCompleter(sharedCompleter);
				c->setCompletionPrefix(completionPrefix);
				if (c->completionCount() == 0) {
					if (cmpCursor.selectionStart() < start) {
						// we must have included a preceding brace or hyphen; now try without it
						cmpCursor.setPosition(start);
						cmpCursor.setPosition(end, QTextCursor::KeepAnchor);
						continue;
					}
					setCompleter(NULL);
				}
				else {
					if (e->modifiers() == Qt::ShiftModifier)
						c->setCurrentRow(c->completionCount() - 1);
					showCurrentCompletion();
				}
			}
			break;
		}
	}
	else if (c->completionCount() > 0) {
		if (e->modifiers() == Qt::ShiftModifier)  {
			if (c->currentRow() == 0) {
				showCompletion(c->completionPrefix());
				setCompleter(NULL);
			}
			else {
				c->setCurrentRow(c->currentRow() - 1);
				showCurrentCompletion();
			}
		}
		else {
			if (c->currentRow() == c->completionCount() - 1) {
				showCompletion(c->completionPrefix());
				setCompleter(NULL);
			}
			else {
				c->setCurrentRow(c->currentRow() + 1);
				showCurrentCompletion();
			}
		}
	}
}

void CompletingEdit::showCompletion(const QString& completion, int insOffset)
{
	disconnect(this, SIGNAL(cursorPositionChanged()), this, SLOT(clearCompleter()));

	if (c->widget() != this)
		return;

	QTextCursor tc = cmpCursor;
	if (tc.isNull()) {
		tc = textCursor();
		tc.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, c->completionPrefix().length());
	}

	tc.insertText(completion);
	cmpCursor = tc;
	cmpCursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, completion.length());

	if (insOffset != -1)
		tc.movePosition(QTextCursor::Left, QTextCursor::MoveAnchor, completion.length() - insOffset);
	setTextCursor(tc);

	ExtraSelection sel;
	sel.cursor = cmpCursor;
	sel.format = QTextCharFormat();
	sel.format.setBackground(QBrush(QColor(255, 255, 127)));
	QList<ExtraSelection> selections;
	selections.append(sel);
	setExtraSelections(selections);

	connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(clearCompleter()));
}

void CompletingEdit::showCurrentCompletion()
{
	if (c->widget() != this)
		return;

	QStandardItemModel *model = qobject_cast<QStandardItemModel*>(c->model());
	QList<QStandardItem*> items = model->findItems(c->currentCompletion());

	if (items.count() > 1) {
		if (c->currentCompletion() == prevCompletion) {
			if (c->currentIndex().row() > prevRow)
				++itemIndex;
			else
				--itemIndex;
			if (itemIndex < 0)
				itemIndex = items.count() - 1;
			else if (itemIndex > items.count() - 1)
				itemIndex = 0;
		}
		else
			itemIndex = 0;
		prevRow = c->currentIndex().row();
		prevCompletion = c->currentCompletion();
	}
	else {
		prevCompletion = QString();
		itemIndex = 0;
	}

	QString completion = model->item(items[itemIndex]->row(), 1)->text();
	
	int insOffset = completion.indexOf("#INS#");
	if (insOffset != -1)
		completion.replace("#INS#", "");

	showCompletion(completion, insOffset);
}

void CompletingEdit::loadCompletionsFromFile(QStandardItemModel *model, const QString& filename)
{
	QFile	completionFile(filename);
	if (completionFile.exists() && completionFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
		QTextStream in(&completionFile);
		in.setCodec("UTF-8");
		in.setAutoDetectUnicode(true);
		QList<QStandardItem*> row;
		while (1) {
			QString	line = in.readLine();
			if (line.isNull())
				break;
			if (line[0] == '%')
				continue;
			line.replace("#RET#", "\n");
			QStringList parts = line.split(":=");
			if (parts.count() > 2)
				continue;
			if (parts.count() == 1)
				parts.append(parts[0]);
			parts[0].replace("#INS#", "");
			row.append(new QStandardItem(parts[0]));
			row.append(new QStandardItem(parts[1]));
			model->appendRow(row);
			row.clear();
		}
		completionFile.close();
	}
}

void CompletingEdit::loadCompletionFiles(QCompleter *theCompleter)
{
	QStandardItemModel *model = new QStandardItemModel(0, 2, theCompleter); // columns are abbrev, expansion

	QDir completionDir(TWUtils::getLibraryPath("completion"));
	foreach (QFileInfo fileInfo, completionDir.entryInfoList(QDir::Files | QDir::Readable, QDir::Name)) {
		loadCompletionsFromFile(model, fileInfo.canonicalFilePath());
	}

	theCompleter->setModel(model);
}

void CompletingEdit::contextMenuEvent(QContextMenuEvent *event)
{
	QMenu *menu = createStandardContextMenu();

	if (pHunspell != NULL) {
		currentWord = cursorForPosition(event->pos());
		currentWord.setPosition(currentWord.position());
		if (selectWord(currentWord)) {
			QByteArray word = spellingCodec->fromUnicode(currentWord.selectedText());
			int spellResult = Hunspell_spell(pHunspell, word.data());
			if (spellResult == 0) {
				char **suggestionList;
				int count = Hunspell_suggest(pHunspell, &suggestionList, word.data());
				menu->insertSeparator(menu->actions().first());
				if (count == 0)
					menu->insertAction(menu->actions().first(), new QAction(tr("No suggestions"), this));
				else {
					QSignalMapper *mapper = new QSignalMapper(menu);
					QAction* sep = menu->actions().first();
					for (int i = 0; i < count; ++i) {
						QString str = spellingCodec->toUnicode(suggestionList[i]);
						QAction *act = new QAction(str, menu);
						connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
						mapper->setMapping(act, str);
						menu->insertAction(sep, act);
						free(suggestionList[i]);
					}
					free(suggestionList);
					connect(mapper, SIGNAL(mapped(const QString&)), this, SLOT(correction(const QString&)));
				}
			}
		}
	}
	
	menu->exec(event->globalPos());
	delete menu;
}

void CompletingEdit::setSpellChecker(Hunhandle* h, QTextCodec *codec)
{
	pHunspell = h;
	spellingCodec = codec;
}

void CompletingEdit::correction(const QString& suggestion)
{
	currentWord.insertText(suggestion);
}

QCompleter	*CompletingEdit::sharedCompleter = NULL;
