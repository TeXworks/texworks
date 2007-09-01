#include "TeXDocument.h"
#include "TeXHighlighter.h"
#include "FindDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStatusBar>
#include <QFontDialog>
#include <QInputDialog>
#include <QSettings>
#include <QStringList>

const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

TeXDocument::TeXDocument()
{
	init();
	setCurrentFile("");
	statusBar()->showMessage(tr("New document"), kStatusMessageDuration);
}

TeXDocument::TeXDocument(const QString &fileName)
{
	init();
	loadFile(fileName);
}

void
TeXDocument::init()
{
	setupUi(this);
	
	textEdit_console->hide();
	
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(actionAbout_QTeX, SIGNAL(triggered()), qApp, SLOT(about()));

	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionClose, SIGNAL(triggered()), this, SLOT(close()));

	connect(actionFont, SIGNAL(triggered()), this, SLOT(doFontDialog()));
	connect(actionGo_to_Line, SIGNAL(triggered()), this, SLOT(doLineDialog()));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(doFindDialog()));

	connect(actionIndent, SIGNAL(triggered()), this, SLOT(doIndent()));
	connect(actionUnindent, SIGNAL(triggered()), this, SLOT(doUnindent()));

	connect(actionComment, SIGNAL(triggered()), this, SLOT(doComment()));
	connect(actionUncomment, SIGNAL(triggered()), this, SLOT(doUncomment()));

	connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));

	for (int i = 0; i < kMaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	menuRecent = new QMenu(tr("Open Recent"), menuFile);
	menuFile->insertMenu(actionOpen_Recent, menuRecent);
	menuFile->removeAction(actionOpen_Recent);
	for (int i = 0; i < kMaxRecentFiles; ++i)
		menuRecent->addAction(recentFileActs[i]);
	updateRecentFileActions();
	
	highlighter = new TeXHighlighter(textEdit->document());
}

void
TeXDocument::newFile()
{
	TeXDocument *doc = new TeXDocument;
	doc->move(x() + kNewWindowOffset, y() + kNewWindowOffset);
	doc->show();
}

void
TeXDocument::open()
{
	QString fileName = QFileDialog::getOpenFileName();
	open(fileName);
}

void TeXDocument::open(const QString &fileName)
{
	if (!fileName.isEmpty()) {
		TeXDocument *doc = findDocument(fileName);
		if (doc) {
			doc->show();
			doc->raise();
			doc->activateWindow();
			return;
		}

		if (isUntitled && textEdit->document()->isEmpty() && !isWindowModified())
			loadFile(fileName);
		else {
			doc = new TeXDocument(fileName);
			if (doc->isUntitled) {
				delete doc;
				return;
			}
			doc->move(x() + kNewWindowOffset, y() + kNewWindowOffset);
			doc->show();
		}
	}
}

void TeXDocument::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		open(action->data().toString());
}

void TeXDocument::closeEvent(QCloseEvent *event)
{
	if (maybeSave())
		event->accept();
	else
		event->ignore();
}

bool TeXDocument::save()
{
	if (isUntitled) {
		return saveAs();
	} else {
		return saveFile(curFile);
	}
}

bool TeXDocument::saveAs()
{
	QString fileName = QFileDialog::getSaveFileName(this);
	if (fileName.isEmpty())
		return false;

	return saveFile(fileName);
}

bool TeXDocument::maybeSave()
{
	if (textEdit->document()->isModified()) {
		QMessageBox::StandardButton ret;
		ret = QMessageBox::warning(this, tr("QTeX"),
					 tr("The document \"%1\" has been modified.\n"
						"Do you want to save your changes?")
						.arg(strippedName(curFile)),
					 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (ret == QMessageBox::Save)
			return save();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	return true;
}

void TeXDocument::loadFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("QTeX"),
							 tr("Cannot read file \"%1\":\n%2.")
							 .arg(fileName)
							 .arg(file.errorString()));
		return;
	}

	QTextStream in(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	textEdit->setPlainText(in.readAll());
	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File \"%1\" loaded").arg(strippedName(curFile)), kStatusMessageDuration);
}

bool TeXDocument::saveFile(const QString &fileName)
{
	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr("QTeX"),
							 tr("Cannot write file \"%1\":\n%2.")
							 .arg(fileName)
							 .arg(file.errorString()));
		return false;
	}

	QTextStream out(&file);
	QApplication::setOverrideCursor(Qt::WaitCursor);
	out << textEdit->toPlainText();
	QApplication::restoreOverrideCursor();

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File \"%1\" saved").arg(strippedName(curFile)), kStatusMessageDuration);
	return true;
}

void TeXDocument::setCurrentFile(const QString &fileName)
{
	static int sequenceNumber = 1;

	isUntitled = fileName.isEmpty();
	if (isUntitled)
		curFile = tr("untitled-%1.tex").arg(sequenceNumber++);
	else
		curFile = QFileInfo(fileName).canonicalFilePath();

	textEdit->document()->setModified(false);
	setWindowModified(false);

	setWindowTitle(tr("%1[*] - %2").arg(strippedName(curFile)).arg(tr("QTeX")));

	if (!isUntitled) {
		QSettings settings;
		QStringList files = settings.value("recentFileList").toStringList();
		files.removeAll(fileName);
		files.prepend(fileName);
		while (files.size() > kMaxRecentFiles)
			files.removeLast();
		settings.setValue("recentFileList", files);

		foreach (QWidget *widget, QApplication::topLevelWidgets()) {
			TeXDocument *doc = qobject_cast<TeXDocument *>(widget);
			if (doc)
				doc->updateRecentFileActions();
		}
	}
}

void TeXDocument::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), kMaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = strippedName(files[i]);
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < kMaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);
}

QString TeXDocument::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

TeXDocument *TeXDocument::findDocument(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QWidget *widget, qApp->topLevelWidgets()) {
		TeXDocument *theDoc = qobject_cast<TeXDocument *>(widget);
		if (theDoc && theDoc->curFile == canonicalFilePath)
			return theDoc;
	}
	return NULL;
}

void TeXDocument::doFontDialog()
{
	textEdit->setFont(QFontDialog::getFont(0, textEdit->font()));
}

void TeXDocument::doLineDialog()
{
	QTextCursor cursor = textEdit->textCursor();

	bool ok;
	int i = QInputDialog::getInteger(this, tr("Go to Line"),
									tr("Line number:"), cursor.blockNumber() + 1,
									1, textEdit->document()->blockCount(), 1, &ok);
	if (ok) {
		cursor.setPosition(0);
		cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, i - 1);
		cursor.select(QTextCursor::BlockUnderCursor);
		textEdit->setTextCursor(cursor);
	}
}

void TeXDocument::doFindDialog()
{
	FindDialog::doFindDialog(this);
}

void TeXDocument::prefixLines(const QString &prefix)
{
	QTextCursor cursor = textEdit->textCursor();
	cursor.beginEditBlock();
	int selStart = cursor.selectionStart();
	int selEnd = cursor.selectionEnd();
	cursor.setPosition(selStart);
	if (!cursor.atBlockStart()) {
		cursor.movePosition(QTextCursor::StartOfBlock);
		selStart = cursor.position();
	}
	cursor.setPosition(selEnd);
	if (!cursor.atBlockStart() || selEnd == selStart) {
		cursor.movePosition(QTextCursor::NextBlock);
		selEnd = cursor.position();
	}
	if (selEnd == selStart)
		goto handle_end_of_doc;	// special case
	while (cursor.position() > selStart) {
		cursor.movePosition(QTextCursor::PreviousBlock);
	handle_end_of_doc:
		cursor.insertText(prefix);
		cursor.movePosition(QTextCursor::StartOfBlock);
		selEnd += prefix.length();
	}
	cursor.setPosition(selStart);
	cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
	textEdit->setTextCursor(cursor);
	cursor.endEditBlock();
}

void TeXDocument::doIndent()
{
	prefixLines("\t");
}

void TeXDocument::doComment()
{
	prefixLines("%");
}

void TeXDocument::unPrefixLines(const QString &prefix)
{
	QTextCursor cursor = textEdit->textCursor();
	cursor.beginEditBlock();
	int selStart = cursor.selectionStart();
	int selEnd = cursor.selectionEnd();
	cursor.setPosition(selStart);
	if (!cursor.atBlockStart()) {
		cursor.movePosition(QTextCursor::StartOfBlock);
		selStart = cursor.position();
	}
	cursor.setPosition(selEnd);
	if (!cursor.atBlockStart() || selEnd == selStart) {
		cursor.movePosition(QTextCursor::NextBlock);
		selEnd = cursor.position();
	}
	while (cursor.position() > selStart) {
		cursor.movePosition(QTextCursor::PreviousBlock);
		cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor);
		QString		str = cursor.selectedText();
		if (str == prefix) {
			cursor.removeSelectedText();
			selEnd -= prefix.length();
		}
		else
			cursor.movePosition(QTextCursor::PreviousCharacter);
	}
	cursor.setPosition(selStart);
	cursor.setPosition(selEnd, QTextCursor::KeepAnchor);
	textEdit->setTextCursor(cursor);
	cursor.endEditBlock();
}

void TeXDocument::doUnindent()
{
	unPrefixLines("\t");
}

void TeXDocument::doUncomment()
{
	unPrefixLines("%");
}
