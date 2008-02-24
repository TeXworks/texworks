#include "CompletingEdit.h"

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
#include <QTextStream>


CompletingEdit::CompletingEdit(QWidget *parent)
	: QTextEdit(parent), c(NULL), cmpCursor(QTextCursor())
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
void CompletingEdit::focusInEvent(QFocusEvent *e)
{
    if (c)
        c->setWidget(this);
    QTextEdit::focusInEvent(e);
}

void CompletingEdit::keyPressEvent(QKeyEvent *e)
{
    bool isShortcut = (e->key() == Qt::Key_Escape);

    if (!isShortcut) {	// not the shortcut key, so simply accept it
		if (e->text() != "") {
			clearCompleter();
			cmpCursor = QTextCursor();
		}
        QTextEdit::keyPressEvent(e);
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
		cmpCursor.select(QTextCursor::WordUnderCursor);
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

	QFileInfo	fileInfo;
	const QString	filename = "TeXworks-completion.txt";
	foreach (const QString& libPath, QCoreApplication::libraryPaths()) {
		QDir	dir(libPath);
#ifdef Q_WS_MAC
		if (dir.dirName() == "MacOS") {
			if (dir.cdUp())
				if (dir.dirName() == "Contents") {
					dir.cdUp(); // up to the .app package dir
					dir.cdUp(); // and to the enclosing dir where the app is located
				}
		}
#endif
		fileInfo = QFileInfo(dir, filename);
		if (fileInfo.exists())
			break;
	}

	if (fileInfo.filePath() != "")
		loadCompletionsFromFile(model, fileInfo.canonicalFilePath());

	theCompleter->setModel(model);
}

QCompleter			*CompletingEdit::sharedCompleter = NULL;
