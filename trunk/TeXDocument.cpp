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

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "TeXDocument.h"
#include "TeXHighlighter.h"
#include "FindDialog.h"
#include "TemplateDialog.h"
#include "TWApp.h"
#include "TWUtils.h"
#include "PDFDocument.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStatusBar>
#include <QFontDialog>
#include <QInputDialog>
#include <QDesktopWidget>
#include <QClipboard>
#include <QSettings>
#include <QStringList>
#include <QUrl>
#include <QComboBox>
#include <QRegExp>
#include <QProcess>
#include <QAbstractItemView>
#include <QScrollBar>
#include <QActionGroup>
#include <QTextCodec>
#include <QSignalMapper>
#include <QDebug>

const int kMinConsoleHeight = 160;

QList<TeXDocument*> TeXDocument::docList;

TeXDocument::TeXDocument()
{
	init();
	makeUntitled();
	statusBar()->showMessage(tr("New document"), kStatusMessageDuration);
}

TeXDocument::TeXDocument(const QString &fileName, bool asTemplate)
{
	init();
	loadFile(fileName, asTemplate);
}

TeXDocument::~TeXDocument()
{
	if (pdfDoc != NULL) {
		pdfDoc->close();
		pdfDoc = NULL;
	}
	docList.removeAll(this);
	updateWindowMenu();
}

void TeXDocument::init()
{
	pdfDoc = NULL;
	process = NULL;
	highlighter = NULL;
	pHunspell = NULL;

	setupUi(this);

	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_MacNoClickThrough, true);

	setContextMenuPolicy(Qt::NoContextMenu);

	hideConsole();

	lineNumberLabel = new QLabel();
	statusBar()->addPermanentWidget(lineNumberLabel);
	lineNumberLabel->setFrameStyle(QFrame::StyledPanel);
	lineNumberLabel->setFont(statusBar()->font());
	statusLine = statusTotal = 0;
	showCursorPosition();
	
	engineActions = new QActionGroup(this);
	connect(engineActions, SIGNAL(triggered(QAction*)), this, SLOT(selectedEngine(QAction*)));
	
	codec = TWApp::instance()->getDefaultCodec();
	engineName = TWApp::instance()->getDefaultEngine().name();
	engine = new QComboBox(this);
	engine->setEditable(false);
	engine->setFocusPolicy(Qt::NoFocus);
	toolBar_run->addWidget(engine);
	updateEngineList();
	connect(engine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectedEngine(const QString&)));
	
	connect(TWApp::instance(), SIGNAL(engineListChanged()), this, SLOT(updateEngineList()));
	
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(actionNew_from_Template, SIGNAL(triggered()), this, SLOT(newFromTemplate()));
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(actionAbout_TW, SIGNAL(triggered()), qApp, SLOT(about()));

	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionClose, SIGNAL(triggered()), this, SLOT(close()));

	connect(actionClear, SIGNAL(triggered()), this, SLOT(clear()));

	connect(actionFont, SIGNAL(triggered()), this, SLOT(doFontDialog()));
	connect(actionGo_to_Line, SIGNAL(triggered()), this, SLOT(doLineDialog()));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(doFindDialog()));
	connect(actionFind_Again, SIGNAL(triggered()), this, SLOT(doFindAgain()));
	connect(actionReplace, SIGNAL(triggered()), this, SLOT(doReplaceDialog()));

	connect(actionCopy_to_Find, SIGNAL(triggered()), this, SLOT(copyToFind()));
	connect(actionCopy_to_Replace, SIGNAL(triggered()), this, SLOT(copyToReplace()));
	connect(actionFind_Selection, SIGNAL(triggered()), this, SLOT(findSelection()));

	connect(actionShow_Selection, SIGNAL(triggered()), this, SLOT(showSelection()));

	connect(actionIndent, SIGNAL(triggered()), this, SLOT(doIndent()));
	connect(actionUnindent, SIGNAL(triggered()), this, SLOT(doUnindent()));

	connect(actionComment, SIGNAL(triggered()), this, SLOT(doComment()));
	connect(actionUncomment, SIGNAL(triggered()), this, SLOT(doUncomment()));

	connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
	connect(textEdit->document(), SIGNAL(modificationChanged(bool)), actionSave, SLOT(setEnabled(bool)));
	connect(textEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
	connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(showCursorPosition()));
	connect(textEdit, SIGNAL(selectionChanged()), this, SLOT(showCursorPosition()));
	connect(textEdit, SIGNAL(syncClick(int)), this, SLOT(syncClick(int)));
	connect(this, SIGNAL(syncFromSource(const QString&, int)), qApp, SLOT(syncFromSource(const QString&, int)));

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
	clipboardChanged();

	connect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->insertMenu(actionOpen_Recent, menuRecent);
	menuFile->removeAction(actionOpen_Recent);

	connect(qApp, SIGNAL(recentFileActionsChanged()), this, SLOT(updateRecentFileActions()));
	connect(qApp, SIGNAL(windowListChanged()), this, SLOT(updateWindowMenu()));
	
	connect(actionStack, SIGNAL(triggered()), qApp, SLOT(stackWindows()));
	connect(actionTile, SIGNAL(triggered()), qApp, SLOT(tileWindows()));
	connect(actionTile_Front_Two, SIGNAL(triggered()), qApp, SLOT(tileTwoWindows()));
	connect(actionShow_Hide_Console, SIGNAL(triggered()), this, SLOT(toggleConsoleVisibility()));
	connect(actionGo_to_Preview, SIGNAL(triggered()), this, SLOT(goToPreview()));
	
	connect(this, SIGNAL(destroyed()), qApp, SLOT(updateWindowMenus()));

	connect(actionPreferences, SIGNAL(triggered()), qApp, SLOT(preferences()));

	connect(menuEdit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

	textEdit->installEventFilter(CmdKeyFilter::filter());

	connect(inputLine, SIGNAL(returnPressed()), this, SLOT(acceptInputLine()));

	QSettings settings;
	TWUtils::applyToolbarOptions(this, settings.value("toolBarIconSize", 2).toInt(), settings.value("toolBarShowText", false).toBool());

	highlighter = new TeXHighlighter(textEdit->document());
	bool b = settings.value("wrapLines", true).toBool();
	actionWrap_Lines->setChecked(b);
	setWrapLines(b);

	b = settings.value("syntaxColoring", true).toBool();
	actionSyntax_Coloring->setChecked(b);
	setSyntaxColoring(b);
	
	connect(actionWrap_Lines, SIGNAL(triggered(bool)), this, SLOT(setWrapLines(bool)));
	connect(actionSyntax_Coloring, SIGNAL(triggered(bool)), this, SLOT(setSyntaxColoring(bool)));

	QSignalMapper *mapper = new QSignalMapper(this);
	connect(actionNone, SIGNAL(triggered()), mapper, SLOT(map()));
	mapper->setMapping(actionNone, QString());
	connect(mapper, SIGNAL(mapped(const QString&)), this, SLOT(setLanguage(const QString&)));

	QActionGroup *group = new QActionGroup(this);
	group->addAction(actionNone);

	QString defLang = settings.value("language", "en_US").toString();
	foreach (QString lang, *TWUtils::getDictionaryList()) {
		QAction *act = menuSpelling->addAction(lang);
		act->setCheckable(true);
		connect(act, SIGNAL(triggered()), mapper, SLOT(map()));
		mapper->setMapping(act, lang);
		group->addAction(act);
		if (lang == defLang)
			act->trigger();
	}
	
	menuShow->addAction(toolBar_run->toggleViewAction());
	menuShow->addAction(toolBar_edit->toggleViewAction());
	
	TWUtils::zoomToHalfScreen(this);

	docList.append(this);
}

void TeXDocument::setLanguage(const QString& lang)
{
	pHunspell = TWUtils::getDictionary(lang);
	if (pHunspell != NULL) {
		codec = QTextCodec::codecForName(Hunspell_get_dic_encoding(pHunspell));
		if (codec == NULL)
			codec = QTextCodec::codecForLocale(); // almost certainly wrong, if we couldn't find the actual name!
	}
	else
		codec = NULL;
	highlighter->setSpellChecker(pHunspell, codec);
	textEdit->setSpellChecker(pHunspell, codec);
}

void TeXDocument::clipboardChanged()
{
	actionPaste->setEnabled(textEdit->canPaste());
}

void TeXDocument::editMenuAboutToShow()
{
//	undoAction->setText(tr("Undo ") + undoStack->undoText());
//	redoAction->setText(tr("Redo ") + undoStack->redoText());
	actionSelect_All->setEnabled(!textEdit->document()->isEmpty());
}

void TeXDocument::newFile()
{
	TeXDocument *doc = new TeXDocument;
	doc->selectWindow();
}

void TeXDocument::newFromTemplate()
{
	QString templateName = TemplateDialog::doTemplateDialog();
	if (!templateName.isEmpty()) {
		TeXDocument *doc = NULL;
		if (isUntitled && textEdit->document()->isEmpty() && !isWindowModified()) {
			loadFile(templateName, true);
			doc = this;
		}
		else {
			doc = new TeXDocument(templateName, true);
		}
		if (doc != NULL) {
			doc->makeUntitled();
			doc->selectWindow();
		}
	}
}

void TeXDocument::makeUntitled()
{
	setCurrentFile("");
}

void TeXDocument::open()
{
	QFileDialog::Options options = 0;
#ifdef Q_WS_MAC
		/* use a sheet if we're calling Open from an empty, untitled, untouched window; otherwise use a separate dialog */
	if (!(isUntitled && textEdit->document()->isEmpty() && !isWindowModified()))
		options = QFileDialog::DontUseSheet;
#endif
	QString fileName = QFileDialog::getOpenFileName(this, QString(tr("Open File")), QString(), QString(), NULL, options);
	TWApp::instance()->open(fileName); // not TeXDocument::open() - give the app a chance to open as PDF
}

TeXDocument* TeXDocument::open(const QString &fileName)
{
	TeXDocument *doc = NULL;
	if (!fileName.isEmpty()) {
		doc = findDocument(fileName);
		if (doc == NULL) {
			if (isUntitled && textEdit->document()->isEmpty() && !isWindowModified()) {
				loadFile(fileName);
				doc = this;
			}
			else {
				doc = new TeXDocument(fileName);
				if (doc->isUntitled) {
					delete doc;
					doc = NULL;
				}
			}
		}
	}
	if (doc != NULL)
		doc->selectWindow();
	return doc;
}

void TeXDocument::openDocument(const QString &fileName, int lineNo) // static
{
	TeXDocument *doc = findDocument(fileName);
	if (doc == NULL) {
		if (docList.count() == 1) {
			doc = docList[0];
			doc = doc->open(fileName); // open into existing window if untitled/empty
		}
		else {
			doc = new TeXDocument(fileName);
			if (doc->isUntitled) {
				delete doc;
				return;
			}
		}
	}
	if (doc != NULL) {
		doc->selectWindow();
		if (lineNo > 0)
			doc->goToLine(lineNo);
	}
}

void TeXDocument::closeEvent(QCloseEvent *event)
{
	if (process != NULL) {
		statusBar()->showMessage(tr("Cannot close window while tool is running"), kStatusMessageDuration);
		event->ignore();
		return;
	}

	if (maybeSave())
		event->accept();
	else
		event->ignore();
}

bool TeXDocument::event(QEvent *event) // based on example at doc.trolltech.com/qq/qq18-macfeatures.html
{
	if (!isActiveWindow())
		return QMainWindow::event(event);

	switch (event->type()) {
		case QEvent::IconDrag:
			{
				event->accept();
				Qt::KeyboardModifiers mods = qApp->keyboardModifiers();
				if (mods == Qt::NoModifier) {
					QDrag *drag = new QDrag(this);
					QMimeData *data = new QMimeData();
					data->setUrls(QList<QUrl>() << QUrl::fromLocalFile(curFile));
					drag->setMimeData(data);
					QPixmap dragIcon(":/images/images/texdoc.png");
					drag->setPixmap(dragIcon);
					drag->setHotSpot(QPoint(dragIcon.width() - 5, 5));
					drag->start(Qt::LinkAction | Qt::CopyAction);
				}
				else if (mods == Qt::ShiftModifier) {
					QMenu menu(this);
					connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(openAt(QAction*)));
					QFileInfo info(curFile);
					QAction *action = menu.addAction(info.fileName());
					action->setIcon(QIcon(":/images/images/texdoc.png"));
					QStringList folders = info.absolutePath().split('/');
					QStringListIterator it(folders);
					it.toBack();
					while (it.hasPrevious()) {
						QString str = it.previous();
						QIcon icon;
						if (!str.isEmpty()) {
							icon = style()->standardIcon(QStyle::SP_DirClosedIcon, 0, this);
						}
						else {
							str = "/";
							icon = style()->standardIcon(QStyle::SP_DriveHDIcon, 0, this);
						}
						action = menu.addAction(str);
						action->setIcon(icon);
					}
					QPoint pos(QCursor::pos().x() - 20, frameGeometry().y());
					menu.exec(pos);
				}
				else {
					event->ignore();
				}
				return true;
			}
		
		default:
			return QMainWindow::event(event);
	}
}

void TeXDocument::openAt(QAction *action)
{
	QString path = curFile.left(curFile.indexOf(action->text())) + action->text();
	if (path == curFile)
		return;
	QProcess proc;
	proc.start("/usr/bin/open", QStringList() << path, QIODevice::ReadOnly);
	proc.waitForFinished();
}

bool TeXDocument::save()
{
	if (isUntitled)
		return saveAs();
	else
		return saveFile(curFile);
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
		ret = QMessageBox::warning(this, tr(TEXWORKS_NAME),
					 tr("The document \"%1\" has been modified.\n"
						"Do you want to save your changes?")
						.arg(TWUtils::strippedName(curFile)),
					 QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
		if (ret == QMessageBox::Save)
			return save();
		else if (ret == QMessageBox::Cancel)
			return false;
	}
	return true;
}

static char* texshopSynonyms[] = {
	"MacOSRoman",		"Apple Roman",
	"IsoLatin",			"ISO 8859-1",
	"IsoLatin2",		"ISO 8859-2",
	"IsoLatin5",		"ISO 8859-5",
	"IsoLatin9",		"ISO 8859-9",
//	"MacJapanese",		"",
//	"DOSJapanese",		"",
	"SJIS_X0213",		"Shift-JIS",
	"EUC_JP",			"EUC-JP",
//	"JISJapanese",		"",
//	"MacKorean",		"",
	"UTF-8 Unicode",	"UTF-8",
	"Standard Unicode",	"UTF-16",
//	"Mac Cyrillic",		"",
//	"DOS Cyrillic",		"",
//	"DOS Russian",		"",
	"Windows Cyrillic",	"Windows-1251",
	"KOI8_R",			"KOI8-R",
//	"Mac Chinese Traditional",	"",
//	"Mac Chinese Simplified",	"",
//	"DOS Chinese Traditional",	"",
//	"DOS Chinese Simplified",	"",
//	"GBK",				"",
//	"GB 2312",			"",
	"GB 18030",			"GB18030-0",
	NULL
};

QTextCodec *TeXDocument::scanForEncoding(const QString &peekStr, bool &hasMetadata, QString &reqName)
{
	// peek at the file for %!TEX encoding = ....
	QRegExp re("%!TEX encoding *= *([^\\r\\n]+)[\\r\\n]", Qt::CaseInsensitive);
	int pos = re.indexIn(peekStr);
	QTextCodec *reqCodec = NULL;
	if (pos > -1) {
		hasMetadata = true;
		reqName = re.cap(1).trimmed();
		reqCodec = QTextCodec::codecForName(reqName.toAscii());
		if (reqCodec == NULL) {
			static QHash<QString,QString> *synonyms = NULL;
			if (synonyms == NULL) {
				synonyms = new QHash<QString,QString>;
				for (int i = 0; texshopSynonyms[i] != NULL; i += 2)
					synonyms->insert(QString(texshopSynonyms[i]).toLower(), texshopSynonyms[i+1]);
			}
			if (synonyms->contains(reqName.toLower()))
				reqCodec = QTextCodec::codecForName(synonyms->value(reqName.toLower()).toAscii());
		}
	}
	else
		hasMetadata = false;
	return reqCodec;
}

#define PEEK_LENGTH 1024

void TeXDocument::loadFile(const QString &fileName, bool asTemplate)
{
	QFile file(fileName);
	if (!file.open(QFile::ReadOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(TEXWORKS_NAME),
							 tr("Cannot read file \"%1\":\n%2.")
							 .arg(fileName)
							 .arg(file.errorString()));
		return;
	}

	QString peekStr(file.peek(PEEK_LENGTH));
	QString reqName;
	bool hasMetadata;
	codec = scanForEncoding(peekStr, hasMetadata, reqName);
	if (codec == NULL) {
		codec = TWApp::instance()->getDefaultCodec();
		if (hasMetadata) {
			if (QMessageBox::warning(this, tr("Unrecognized encoding"),
					tr("The text encoding %1 used in %2 is not supported.\n\n"
					   "It will be interpreted as %3 instead, which may result in incorrect text.")
						.arg(reqName)
						.arg(fileName)
						.arg(QString::fromAscii(codec->name())),
					QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
				return;
		}
	}
	
	QTextStream in(&file);
	in.setCodec(codec);

	QApplication::setOverrideCursor(Qt::WaitCursor);
	textEdit->setPlainText(in.readAll());
	QApplication::restoreOverrideCursor();

	if (!asTemplate) {
		setCurrentFile(fileName);
		showPdfIfAvailable();
		selectWindow();
		
		statusBar()->showMessage(tr("File \"%1\" loaded (%2)")
									.arg(TWUtils::strippedName(curFile))
									.arg(QString::fromAscii(codec->name())),
									kStatusMessageDuration);
	}
}

void TeXDocument::showPdfIfAvailable()
{
	findRootFilePath();
	if (rootFilePath == "")
		return;
	QFileInfo fi(rootFilePath);
	QString pdfName = fi.canonicalPath() + "/" + fi.completeBaseName() + ".pdf";
	PDFDocument *pdfDoc = PDFDocument::findDocument(pdfName);
	if (pdfDoc != NULL) {
		pdfDoc->reload();
		pdfDoc->selectWindow();
	}
	else {
		fi.setFile(pdfName);
		if (fi.exists()) {
			pdfDoc = new PDFDocument(pdfName, this);
			TWUtils::sideBySide(this, pdfDoc);
			pdfDoc->show();
			connect(pdfDoc, SIGNAL(destroyed()), this, SLOT(pdfClosed()));
		}
	}
}

void TeXDocument::pdfClosed()
{
	pdfDoc = NULL;
}

bool TeXDocument::saveFile(const QString &fileName)
{
	bool hasMetadata;
	QString reqName;
	QString theText = textEdit->toPlainText();
	QTextCodec *newCodec = scanForEncoding(theText.toAscii().left(PEEK_LENGTH), hasMetadata, reqName);

	if (newCodec != NULL)
		codec = newCodec;
	else if (hasMetadata) {
		if (QMessageBox::warning(this, tr("Unrecognized encoding"),
				tr("The text encoding %1 requested for %2 is not supported.\n\n"
				   "It will be saved as %3 instead, which may result in incorrect text.")
					.arg(reqName)
					.arg(fileName)
					.arg(QString(codec->name())),
				QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
			return false;
	}

	QFile file(fileName);
	if (!file.open(QFile::WriteOnly | QFile::Text)) {
		QMessageBox::warning(this, tr(TEXWORKS_NAME),
							 tr("Cannot write file \"%1\":\n%2.")
							 .arg(fileName)
							 .arg(file.errorString()));
		return false;
	}

	QApplication::setOverrideCursor(Qt::WaitCursor);
	QTextStream out(&file);
	if (codec != NULL)
		out.setCodec(codec);
	out << theText;
	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File \"%1\" saved (%2)")
								.arg(TWUtils::strippedName(curFile))
								.arg(QString::fromAscii(codec ? codec->name() : "default encoding")),
								kStatusMessageDuration);
	QApplication::restoreOverrideCursor();

	return true;
}

void TeXDocument::setCurrentFile(const QString &fileName)
{
	static int sequenceNumber = 1;

	isUntitled = fileName.isEmpty();
	if (isUntitled) {
		curFile = tr("untitled-%1.tex").arg(sequenceNumber++);
		setWindowIcon(QIcon());
	}
	else {
		curFile = QFileInfo(fileName).canonicalFilePath();
		setWindowIcon(QIcon(":/images/images/texdoc.png"));
	}
	
	textEdit->document()->setModified(false);
	setWindowModified(false);

	setWindowTitle(tr("%1[*] - %2").arg(TWUtils::strippedName(curFile)).arg(tr(TEXWORKS_NAME)));

	if (!isUntitled) {
		QSettings settings;
		QStringList files = settings.value("recentFileList").toStringList();
		files.removeAll(fileName);
		files.prepend(fileName);
		while (files.size() > TWApp::instance()->maxRecentFiles())
			files.removeLast();
		settings.setValue("recentFileList", files);
		TWApp::instance()->updateRecentFileActions();
	}
	
	TWApp::instance()->updateWindowMenus();
}

void TeXDocument::updateRecentFileActions()
{
	TWUtils::updateRecentFileActions(this, recentFileActions, menuRecent);
}

void TeXDocument::updateWindowMenu()
{
	TWUtils::updateWindowMenu(this, menuWindow);
}

void TeXDocument::updateEngineList()
{
	while (menuRun->actions().count() > 2)
		menuRun->removeAction(menuRun->actions().last());
	while (engineActions->actions().count() > 0)
		engineActions->removeAction(engineActions->actions().last());
	engine->clear();
	foreach (Engine e, TWApp::instance()->getEngineList()) {
		QAction *newAction = new QAction(e.name(), engineActions);
		newAction->setCheckable(true);
		menuRun->addAction(newAction);
		engine->addItem(e.name());
		if (e.name() == engineName) {
			engine->setCurrentIndex(engine->count() - 1);
			newAction->setChecked(true);
		}
	}
}

void TeXDocument::selectedEngine(QAction* engineAction) // sent by actions in menubar menu; update toolbar combo box
{
	engineName = engineAction->text();
	for (int i = 0; i < engine->count(); ++i)
		if (engine->itemText(i) == engineName) {
			engine->setCurrentIndex(i);
			break;
		}
}

void TeXDocument::selectedEngine(const QString& name) // sent by toolbar combo box; need to update menu
{
	engineName = name;
	foreach (QAction *act, engineActions->actions()) {
		if (act->text() == name) {
			act->setChecked(true);
			break;
		}
	}
}

void TeXDocument::showCursorPosition()
{
	int line = textEdit->textCursor().blockNumber() + 1;
	int total = textEdit->document()->blockCount();
	if (line != statusLine || total != statusTotal) {
		lineNumberLabel->setText(tr("Line %1 of %2").arg(line).arg(total));
		statusLine = line;
		statusTotal = total;
	}
}

void TeXDocument::selectWindow()
{
	show();
	raise();
	activateWindow();
	if (isMinimized())
		showNormal();
}

TeXDocument *TeXDocument::findDocument(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();

	foreach (QWidget *widget, qApp->topLevelWidgets()) {
		TeXDocument *theDoc = qobject_cast<TeXDocument*>(widget);
		if (theDoc && theDoc->curFile == canonicalFilePath)
			return theDoc;
	}
	return NULL;
}

void TeXDocument::clear()
{
	textEdit->textCursor().removeSelectedText();
}

void TeXDocument::goToLine(int lineNo)
{
	QTextCursor cursor = textEdit->textCursor();
	cursor.setPosition(0);
	cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, lineNo - 1);
	cursor.movePosition(QTextCursor::EndOfBlock, QTextCursor::KeepAnchor);
	int oldScrollValue = -1;
	if (textEdit->verticalScrollBar() != NULL)
		oldScrollValue = textEdit->verticalScrollBar()->value();
	textEdit->setTextCursor(cursor);
	maybeCenterSelection(oldScrollValue);
}

void TeXDocument::maybeCenterSelection(int oldScrollValue)
{
	if (oldScrollValue != -1 && textEdit->verticalScrollBar() != NULL) {
		int newScrollValue = textEdit->verticalScrollBar()->value();
		if (newScrollValue != oldScrollValue) {
			int delta = (textEdit->height() - textEdit->cursorRect().height()) / 2;
			if (newScrollValue < oldScrollValue)
				delta = -delta;
			textEdit->verticalScrollBar()->setValue(newScrollValue + delta);
		}
	}
}

void TeXDocument::doFontDialog()
{
	textEdit->setFont(QFontDialog::getFont(0, textEdit->font()));
}

void TeXDocument::doLineDialog()
{
	QTextCursor cursor = textEdit->textCursor();
	bool ok;
	int lineNo = QInputDialog::getInteger(this, tr("Go to Line"),
									tr("Line number:"), cursor.blockNumber() + 1,
									1, textEdit->document()->blockCount(), 1, &ok);
	if (ok)
		goToLine(lineNo);
}

void TeXDocument::doFindDialog()
{
	if (FindDialog::doFindDialog(textEdit) == QDialog::Accepted)
		doFindAgain();
}

void TeXDocument::doReplaceDialog()
{
	ReplaceDialog::DialogCode result;
	if ((result = ReplaceDialog::doReplaceDialog(textEdit)) != ReplaceDialog::Cancel)
		doReplace(result);
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

void TeXDocument::setWrapLines(bool wrap)
{
	textEdit->setWordWrapMode(wrap ? QTextOption::WordWrap : QTextOption::NoWrap);
}

void TeXDocument::setSyntaxColoring(bool coloring)
{
	highlighter->setActive(coloring);
}

void TeXDocument::doFindAgain()
{
	QSettings settings;
	QString	searchText = settings.value("searchText").toString();
	if (searchText.isEmpty())
		return;

	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();

	QRegExp	*regex = NULL;
	if (settings.value("searchRegex").toBool()) {
		regex = new QRegExp(searchText, ((flags & QTextDocument::FindCaseSensitively) != 0)
										? Qt::CaseSensitive : Qt::CaseInsensitive);
		if (!regex->isValid()) {
			qApp->beep();
			statusBar()->showMessage(tr("Invalid regular expression"), kStatusMessageDuration);
			delete regex;
			return;
		}
	}

	QTextCursor	curs = textEdit->textCursor();
	if (settings.value("searchSelection").toBool() && curs.hasSelection()) {
		int s = curs.selectionStart();
		int e = curs.selectionEnd();
		if ((flags & QTextDocument::FindBackward) != 0) {
			curs = (regex != NULL)
				? textEdit->document()->find(*regex, e, flags)
				: textEdit->document()->find(searchText, e, flags);
			if (!curs.isNull()) {
				if (curs.selectionEnd() > e)
					curs = (regex != NULL)
						? textEdit->document()->find(*regex, curs, flags)
						: textEdit->document()->find(searchText, curs, flags);
				if (curs.selectionStart() < s)
					curs = QTextCursor();
			}
		}
		else {
			curs = textEdit->document()->find(searchText, s, flags);
			if (curs.selectionEnd() > e)
				curs = QTextCursor();
		}
	}
	else {
		curs = (regex != NULL)
			? textEdit->document()->find(*regex, curs, flags)
			: textEdit->document()->find(searchText, curs, flags);
		if (curs.isNull() && settings.value("searchWrap").toBool()) {
			curs = QTextCursor(textEdit->document());
			if ((flags & QTextDocument::FindBackward) != 0)
				curs.movePosition(QTextCursor::End);
			curs = (regex != NULL)
				? textEdit->document()->find(*regex, curs, flags)
				: textEdit->document()->find(searchText, curs, flags);
		}
	}

	if (regex != NULL)
		delete regex;

	if (curs.isNull()) {
		qApp->beep();
		statusBar()->showMessage(tr("Not found"), kStatusMessageDuration);
	}
	else
		textEdit->setTextCursor(curs);
}

void TeXDocument::doReplace(ReplaceDialog::DialogCode mode)
{
	QSettings settings;
	
	QString	searchText = settings.value("searchText").toString();
	if (searchText.isEmpty())
		return;
	
	QString	replacement = settings.value("replaceText").toString();
	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();

	QRegExp	*regex = NULL;
	if (settings.value("searchRegex").toBool()) {
		regex = new QRegExp(searchText, ((flags & QTextDocument::FindCaseSensitively) != 0)
										? Qt::CaseSensitive : Qt::CaseInsensitive);
		if (!regex->isValid()) {
			qApp->beep();
			statusBar()->showMessage(tr("Invalid regular expression"), kStatusMessageDuration);
			delete regex;
			return;
		}
	}
	
	bool searchWrap = settings.value("searchWrap").toBool();
	bool searchSel = settings.value("searchSelection").toBool();
	
	int rangeStart, rangeEnd;
	QTextCursor searchRange = textCursor();
	if (searchSel) {
		rangeStart = searchRange.selectionStart();
		rangeEnd = searchRange.selectionEnd();
	}
	else {
		if (searchWrap) {
			searchRange.select(QTextCursor::Document);
			rangeStart = searchRange.selectionStart();
			rangeEnd = searchRange.selectionEnd();
		}
		else {
			if ((flags & QTextDocument::FindBackward) != 0) {
				rangeStart = 0;
				rangeEnd = searchRange.selectionEnd();
			}
			else {
				rangeStart = searchRange.selectionStart();
				searchRange.select(QTextCursor::Document);
				rangeEnd = searchRange.selectionEnd();
			}
		}
	}
	
	if (mode == ReplaceDialog::ReplaceOne) {
		QTextCursor curs = doSearch(searchText, regex, flags, rangeStart, rangeEnd);
		if (curs.isNull()) {
			qApp->beep();
			statusBar()->showMessage(tr("Not found"), kStatusMessageDuration);
		}
		else {
			// do replacement
			QString target = curs.selectedText();
			if (regex != NULL)
				target.replace(*regex, replacement);
			else
				target = replacement;
			curs.insertText(target);
		}
	}
	else if (mode == ReplaceDialog::ReplaceAll) {
		int replacements = 0;
		bool first = true;
		while (1) {
			QTextCursor curs = doSearch(searchText, regex, flags, rangeStart, rangeEnd);
			if (curs.isNull()) {
				if (!first)
					searchRange.endEditBlock();
				break;
			}
			if (first) {
				searchRange.beginEditBlock();
				first = false;
			}
			QString target = curs.selectedText();
			int oldLen = target.length();
			if (regex != NULL)
				target.replace(*regex, replacement);
			else
				target = replacement;
			int newLen = target.length();
			if ((flags & QTextDocument::FindBackward) != 0)
				rangeEnd = curs.selectionStart();
			else {
				rangeStart = curs.selectionEnd() - oldLen + newLen;
				rangeEnd += newLen - oldLen;
			}
			searchRange.setPosition(curs.selectionStart());
			searchRange.setPosition(curs.selectionEnd(), QTextCursor::KeepAnchor);
			searchRange.insertText(target);
			++replacements;
		}
		statusBar()->showMessage(tr("Replaced %1 occurrence(s)").arg(replacements), kStatusMessageDuration);
	}

	if (regex != NULL)
		delete regex;
}

QTextCursor TeXDocument::doSearch(const QString& searchText, const QRegExp *regex, QTextDocument::FindFlags flags, int s, int e)
{
	QTextCursor curs;
	if ((flags & QTextDocument::FindBackward) != 0) {
		if (regex != NULL)
			curs = textEdit->document()->find(*regex, e, flags);
		else
			curs = textEdit->document()->find(searchText, e, flags);
		if (!curs.isNull()) {
			if (curs.selectionEnd() > e) {
				if (regex != NULL)
					curs = textEdit->document()->find(*regex, curs, flags);
				else
					curs = textEdit->document()->find(searchText, curs, flags);
			}
			if (curs.selectionStart() < s)
				curs = QTextCursor();
		}
	}
	else {
		if (regex != NULL)
			curs = textEdit->document()->find(*regex, s, flags);
		else
			curs = textEdit->document()->find(searchText, s, flags);
		if (curs.selectionEnd() > e)
			curs = QTextCursor();
	}
	return curs;
}

void TeXDocument::copyToFind()
{
	if (textEdit->textCursor().hasSelection()) {
		QString searchText = textEdit->textCursor().selectedText();
		QSettings settings;
		if (settings.value("searchRegex").toBool())
			searchText = QRegExp::escape(searchText);
		settings.setValue("searchText", searchText);
	}
}

void TeXDocument::copyToReplace()
{
	if (textEdit->textCursor().hasSelection()) {
		QString replaceText = textEdit->textCursor().selectedText();
		QSettings settings;
		settings.setValue("replaceText", replaceText);
	}
}

void TeXDocument::findSelection()
{
	copyToFind();
	doFindAgain();
}

void TeXDocument::showSelection()
{
	int oldScrollValue = -1;
	if (textEdit->verticalScrollBar() != NULL)
		oldScrollValue = textEdit->verticalScrollBar()->value();
	textEdit->ensureCursorVisible();
	maybeCenterSelection(oldScrollValue);
}

void TeXDocument::zoomToLeft(QWidget *otherWindow)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(otherWindow == NULL ? this : otherWindow);
	screenRect.setTop(screenRect.top() + 22);
	screenRect.setLeft(screenRect.left() + 1);
	screenRect.setBottom(screenRect.bottom() - 1);
	screenRect.setRight((screenRect.left() + screenRect.right()) / 2 - 1);
	setGeometry(screenRect);
}

void TeXDocument::typeset()
{
	if (process)
		return;	// this shouldn't happen if we disable the command at the right time

	if (isUntitled || textEdit->document()->isModified())
		if (!save()) {
			statusBar()->showMessage(tr("Cannot process unsaved document"), kStatusMessageDuration);
			return;
		}

	findRootFilePath();
	QFileInfo fileInfo(rootFilePath);
	if (!fileInfo.isReadable()) {
		statusBar()->showMessage(tr("File %1 is not readable").arg(rootFilePath), kStatusMessageDuration);
		return;
	}

	if (rootFilePath != QFileInfo(curFile).canonicalFilePath()) {
		TeXDocument* rootDoc = findDocument(rootFilePath);
		if (rootDoc != NULL) {
			rootDoc->typeset();
			return;
		}
	}

	Engine e = TWApp::instance()->getNamedEngine(engine->currentText());
	if (e.program() == "") {
		statusBar()->showMessage(tr("%1 is not properly configured").arg(engine->currentText()), kStatusMessageDuration);
		return;
	}

	process = new QProcess(this);
	updateTypesettingAction();
	
	process->setWorkingDirectory(fileInfo.canonicalPath());

	const QStringList& binPaths = TWApp::instance()->getBinaryPaths();
	QStringList env = QProcess::systemEnvironment();
	QStringListIterator iter(binPaths);
	iter.toBack();
	while (iter.hasPrevious()) {
		QString path = iter.previous();
		env.replaceInStrings(QRegExp("^PATH=(.*)", Qt::CaseInsensitive), "PATH=" + path + ":\\1");
	}
	process->setEnvironment(env);
	process->setProcessChannelMode(QProcess::MergedChannels);

	connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(processStandardOutput()));
	connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));

	QStringList args = e.arguments();
	args.replaceInStrings("$fullname", fileInfo.fileName());
	args.replaceInStrings("$basename", fileInfo.completeBaseName());
	args.replaceInStrings("$suffix", fileInfo.suffix());
	args.replaceInStrings("$directory", fileInfo.absoluteDir().absolutePath());

	bool foundCommand = false;
	iter.toFront();
	while (iter.hasNext() && !foundCommand) {
		QString path = iter.next();
		fileInfo = QFileInfo(path, e.program());
		if (fileInfo.exists())
			foundCommand = true;
	}

	if (foundCommand) {
		textEdit_console->clear();
		showConsole();
		inputLine->setFocus(Qt::OtherFocusReason);
		process->start(fileInfo.absoluteFilePath(), args);
	}
	else {
		process->deleteLater();
		process = NULL;
		QMessageBox::critical(this, tr("Unable to execute %1").arg(e.name()),
								tr("The program \"%1\" was not found.\n\n"
								"Check configuration of the %2 tool and path settings"
								" in the Preferences dialog.").arg(e.program()).arg(e.name()),
								QMessageBox::Cancel);
		updateTypesettingAction();
	}
}

void TeXDocument::interrupt()
{
	if (process != NULL) {
		process->kill();
	}
}

void TeXDocument::updateTypesettingAction()
{
	if (process == NULL) {
		disconnect(actionTypeset, SIGNAL(triggered()), this, SLOT(interrupt()));
		actionTypeset->setIcon(QIcon(":/images/images/typeset.png"));
		connect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));
		if (pdfDoc != NULL)
			pdfDoc->updateTypesettingAction(false);
	}
	else {
		disconnect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));
		actionTypeset->setIcon(QIcon(":/images/tango/process-stop.png"));
		connect(actionTypeset, SIGNAL(triggered()), this, SLOT(interrupt()));
		if (pdfDoc != NULL)
			pdfDoc->updateTypesettingAction(true);
	}
}

void TeXDocument::processStandardOutput()
{
	QByteArray bytes = process->readAllStandardOutput();
	QTextCursor cursor(textEdit_console->document());
	cursor.select(QTextCursor::Document);
	cursor.setPosition(cursor.selectionEnd());
	cursor.insertText(QString::fromUtf8(bytes));
	textEdit_console->setTextCursor(cursor);
}

void TeXDocument::processError(QProcess::ProcessError /*error*/)
{
	textEdit_console->append(process->errorString());
	process->kill();
	process->deleteLater();
	process = NULL;
	inputLine->hide();
	updateTypesettingAction();
}

void TeXDocument::processFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
	if (exitStatus != QProcess::CrashExit) {
		if (pdfDoc == NULL) {
			showPdfIfAvailable();
			if (pdfDoc != NULL)
				pdfDoc->selectWindow();
		}
		else {
			pdfDoc->reload();
			pdfDoc->selectWindow();
		}
	}

	QSettings settings;
	if (exitCode == 0 && settings.value("autoHideConsole", true).toBool())
		hideConsole();
	else
		inputLine->hide();

	process->deleteLater();
	process = NULL;
	updateTypesettingAction();
}

void TeXDocument::showConsole()
{
	QList<int> sizes = splitter->sizes();
	int half = (sizes[0] + sizes[1]) / 2;
	if (sizes[1] < kMinConsoleHeight)
		sizes[1] = kMinConsoleHeight > half ? half : kMinConsoleHeight;
	splitter->setSizes(sizes);
	textEdit_console->show();
	if (process != NULL)
		inputLine->show();
	actionShow_Hide_Console->setText(tr("Hide Output Panel"));
}

void TeXDocument::hideConsole()
{
	textEdit_console->hide();
	inputLine->hide();
	actionShow_Hide_Console->setText(tr("Show Output Panel"));
}

void TeXDocument::toggleConsoleVisibility()
{
	if (textEdit_console->isVisible())
		hideConsole();
	else
		showConsole();
}

void TeXDocument::acceptInputLine()
{
	if (process != NULL) {
		QString	str = inputLine->text();
		QTextCursor	curs(textEdit_console->document());
		curs.setPosition(textEdit_console->toPlainText().length());
		textEdit_console->setTextCursor(curs);
		QTextCharFormat	consoleFormat = textEdit_console->currentCharFormat();
		QTextCharFormat inputFormat(consoleFormat);
		inputFormat.setForeground(inputLine->palette().text());
		str.append("\n");
		textEdit_console->insertPlainText(str);
		curs.movePosition(QTextCursor::PreviousCharacter);
		curs.movePosition(QTextCursor::PreviousCharacter, QTextCursor::KeepAnchor, str.length() - 1);
		curs.setCharFormat(inputFormat);
		process->write(str.toUtf8());
		inputLine->clear();
	}
}

void TeXDocument::goToPreview()
{
	if (pdfDoc != NULL)
		pdfDoc->selectWindow();
}

void TeXDocument::syncClick(int lineNo)
{
	if (!isUntitled)
		emit syncFromSource(curFile, lineNo);
}

void TeXDocument::contentsChanged(int position, int /*charsRemoved*/, int /*charsAdded*/)
{
	if (position < PEEK_LENGTH) {
		QTextCursor curs(textEdit->document());
		curs.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, PEEK_LENGTH);
		QString peekStr = curs.selectedText();
		QRegExp re("%!TEX (?:TS-)?program *= *([^\\x2029]+)\\x2029", Qt::CaseInsensitive);
		int pos = re.indexIn(peekStr);
		if (pos > -1) {
			QString name = re.cap(1).trimmed();
			int index = engine->findText(name, Qt::MatchFixedString);
			if (index > -1) {
				if (index != engine->currentIndex()) {
					engine->setCurrentIndex(index);
					statusBar()->showMessage(tr("Set engine to \"%1\"").arg(engine->currentText()), kStatusMessageDuration);
				}
				else
					statusBar()->clearMessage();
			}
			else {
				statusBar()->showMessage(tr("Engine \"%1\" not defined").arg(name), kStatusMessageDuration);
			}
		}
	}
}

void TeXDocument::findRootFilePath()
{
	if (isUntitled) {
		rootFilePath = "";
		return;
	}
	QFileInfo fileInfo(curFile);
	QString rootName;
	QTextCursor curs(textEdit->document());
	curs.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, PEEK_LENGTH);
	QString peekStr = curs.selectedText();
	QRegExp re("%!TEX root *= *([^\\x2029]+)\\x2029", Qt::CaseInsensitive);
	int pos = re.indexIn(peekStr);
	if (pos > -1) {
		rootName = re.cap(1).trimmed();
		QFileInfo rootFileInfo(fileInfo.canonicalPath() + "/" + rootName);
		rootFilePath = rootFileInfo.canonicalFilePath();
	}
	else
		rootFilePath = fileInfo.canonicalFilePath();
}
