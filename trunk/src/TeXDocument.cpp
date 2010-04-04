/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2010  Jonathan Kew

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

#include "TeXDocument.h"
#include "TeXHighlighter.h"
#include "TeXDocks.h"
#include "FindDialog.h"
#include "TemplateDialog.h"
#include "TWApp.h"
#include "TWUtils.h"
#include "PDFDocument.h"
#include "ConfirmDelete.h"
#include "HardWrapDialog.h"
#include "PrefsDialog.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QStatusBar>
#include <QFontDialog>
#include <QInputDialog>
#include <QDesktopWidget>
#include <QClipboard>
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
#include <QDockWidget>
#include <QTableView>
#include <QHeaderView>
#include <QStandardItemModel>
#include <QAbstractButton>
#include <QPushButton>
#include <QFileSystemWatcher>
#include <QDebug>

#ifdef Q_WS_WIN
#include <windows.h>
#endif

#define kLineEnd_Mask   0x00FF
#define kLineEnd_LF     0x0000
#define kLineEnd_CRLF   0x0001
#define kLineEnd_CR     0x0002

#define kLineEnd_Flags_Mask  0xFF00
#define kLineEnd_Mixed       0x0100

const int kHardWrapDefaultWidth = 64;

QList<TeXDocument*> TeXDocument::docList;

TeXDocument::TeXDocument()
{
	init();
	statusBar()->showMessage(tr("New document"), kStatusMessageDuration);
}

TeXDocument::TeXDocument(const QString &fileName, bool asTemplate)
{
	init();
	loadFile(fileName, asTemplate);
}

TeXDocument::~TeXDocument()
{
	docList.removeAll(this);
	updateWindowMenu();
}

void TeXDocument::init()
{
	codec = TWApp::instance()->getDefaultCodec();
	pdfDoc = NULL;
	process = NULL;
	highlighter = NULL;
	pHunspell = NULL;
#ifdef Q_WS_WIN
	lineEndings = kLineEnd_CRLF;
#else
	lineEndings = kLineEnd_LF;
#endif
	
	setupUi(this);
#ifdef Q_WS_WIN
	TWApp::instance()->createMessageTarget(this);
#endif

	setAttribute(Qt::WA_DeleteOnClose, true);
	setAttribute(Qt::WA_MacNoClickThrough, true);

	setContextMenuPolicy(Qt::NoContextMenu);

	makeUntitled();
	hideConsole();
	keepConsoleOpen = false;

	statusBar()->addPermanentWidget(lineEndingLabel = new QLabel());
	lineEndingLabel->setFrameStyle(QFrame::StyledPanel);
	lineEndingLabel->setFont(statusBar()->font());
	lineEndingLabel->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(lineEndingLabel, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(lineEndingPopup(const QPoint)));
	showLineEndingSetting();
	
	statusBar()->addPermanentWidget(encodingLabel = new QLabel());
	encodingLabel->setFrameStyle(QFrame::StyledPanel);
	encodingLabel->setFont(statusBar()->font());
	encodingLabel->setContextMenuPolicy(Qt::CustomContextMenu);
	connect(encodingLabel, SIGNAL(customContextMenuRequested(const QPoint)), this, SLOT(encodingPopup(const QPoint)));
	showEncodingSetting();
	
	statusBar()->addPermanentWidget(lineNumberLabel = new QLabel());
	lineNumberLabel->setFrameStyle(QFrame::StyledPanel);
	lineNumberLabel->setFont(statusBar()->font());
	showCursorPosition();
	
	engineActions = new QActionGroup(this);
	connect(engineActions, SIGNAL(triggered(QAction*)), this, SLOT(selectedEngine(QAction*)));
	
	codec = TWApp::instance()->getDefaultCodec();
	engineName = TWApp::instance()->getDefaultEngine().name();
	engine = new QComboBox(this);
	engine->setEditable(false);
	engine->setFocusPolicy(Qt::NoFocus);
#if defined(Q_WS_MAC) && (QT_VERSION >= 0x040600)
	engine->setStyleSheet("padding:4px;");
#endif
	toolBar_run->addWidget(engine);
	updateEngineList();
	connect(engine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectedEngine(const QString&)));
	
	connect(TWApp::instance(), SIGNAL(engineListChanged()), this, SLOT(updateEngineList()));
	
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));
	connect(actionNew_from_Template, SIGNAL(triggered()), this, SLOT(newFromTemplate()));
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));
	connect(actionAbout_TW, SIGNAL(triggered()), qApp, SLOT(about()));
	connect(actionGoToHomePage, SIGNAL(triggered()), qApp, SLOT(goToHomePage()));
	connect(actionWriteToMailingList, SIGNAL(triggered()), qApp, SLOT(writeToMailingList()));

	connect(actionSave, SIGNAL(triggered()), this, SLOT(save()));
	connect(actionSave_As, SIGNAL(triggered()), this, SLOT(saveAs()));
	connect(actionSave_All, SIGNAL(triggered()), this, SLOT(saveAll()));
	connect(actionRevert_to_Saved, SIGNAL(triggered()), this, SLOT(revert()));
	connect(actionClose, SIGNAL(triggered()), this, SLOT(close()));

	connect(actionRemove_Aux_Files, SIGNAL(triggered()), this, SLOT(removeAuxFiles()));

	connect(actionQuit_TeXworks, SIGNAL(triggered()), TWApp::instance(), SLOT(maybeQuit()));
	
	connect(actionClear, SIGNAL(triggered()), this, SLOT(clear()));

	connect(actionFont, SIGNAL(triggered()), this, SLOT(doFontDialog()));
	connect(actionGo_to_Line, SIGNAL(triggered()), this, SLOT(doLineDialog()));
	connect(actionFind, SIGNAL(triggered()), this, SLOT(doFindDialog()));
	connect(actionFind_Again, SIGNAL(triggered()), this, SLOT(doFindAgain()));
	connect(actionReplace, SIGNAL(triggered()), this, SLOT(doReplaceDialog()));
	connect(actionReplace_Again, SIGNAL(triggered()), this, SLOT(doReplaceAgain()));

	connect(actionCopy_to_Find, SIGNAL(triggered()), this, SLOT(copyToFind()));
	connect(actionCopy_to_Replace, SIGNAL(triggered()), this, SLOT(copyToReplace()));
	connect(actionFind_Selection, SIGNAL(triggered()), this, SLOT(findSelection()));

	connect(actionShow_Selection, SIGNAL(triggered()), this, SLOT(showSelection()));

	connect(actionIndent, SIGNAL(triggered()), this, SLOT(doIndent()));
	connect(actionUnindent, SIGNAL(triggered()), this, SLOT(doUnindent()));

	connect(actionComment, SIGNAL(triggered()), this, SLOT(doComment()));
	connect(actionUncomment, SIGNAL(triggered()), this, SLOT(doUncomment()));

	connect(actionHard_Wrap, SIGNAL(triggered()), this, SLOT(doHardWrapDialog()));
	
	connect(actionTo_Uppercase, SIGNAL(triggered()), this, SLOT(toUppercase()));
	connect(actionTo_Lowercase, SIGNAL(triggered()), this, SLOT(toLowercase()));
	connect(actionToggle_Case, SIGNAL(triggered()), this, SLOT(toggleCase()));

	connect(actionBalance_Delimiters, SIGNAL(triggered()), this, SLOT(balanceDelimiters()));

	connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(setWindowModified(bool)));
	connect(textEdit->document(), SIGNAL(modificationChanged(bool)), this, SLOT(maybeEnableSaveAndRevert(bool)));
	connect(textEdit->document(), SIGNAL(contentsChange(int,int,int)), this, SLOT(contentsChanged(int,int,int)));
	connect(textEdit, SIGNAL(cursorPositionChanged()), this, SLOT(showCursorPosition()));
	connect(textEdit, SIGNAL(selectionChanged()), this, SLOT(showCursorPosition()));
	connect(textEdit, SIGNAL(syncClick(int)), this, SLOT(syncClick(int)));
	connect(this, SIGNAL(syncFromSource(const QString&, int, bool)), qApp, SIGNAL(syncPdf(const QString&, int, bool)));

	connect(QApplication::clipboard(), SIGNAL(dataChanged()), this, SLOT(clipboardChanged()));
	clipboardChanged();

	connect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));

	menuRecent = new QMenu(tr("Open Recent"), this);
	updateRecentFileActions();
	menuFile->insertMenu(actionOpen_Recent, menuRecent);
	menuFile->removeAction(actionOpen_Recent);

	connect(qApp, SIGNAL(recentFileActionsChanged()), this, SLOT(updateRecentFileActions()));
	connect(qApp, SIGNAL(windowListChanged()), this, SLOT(updateWindowMenu()));
	
	connect(qApp, SIGNAL(hideFloatersExcept(QWidget*)), this, SLOT(hideFloatersUnlessThis(QWidget*)));
	connect(this, SIGNAL(activatedWindow(QWidget*)), qApp, SLOT(activatedWindow(QWidget*)));

	connect(actionStack, SIGNAL(triggered()), qApp, SLOT(stackWindows()));
	connect(actionTile, SIGNAL(triggered()), qApp, SLOT(tileWindows()));
	connect(actionSide_by_Side, SIGNAL(triggered()), this, SLOT(sideBySide()));
	connect(actionPlace_on_Left, SIGNAL(triggered()), this, SLOT(placeOnLeft()));
	connect(actionPlace_on_Right, SIGNAL(triggered()), this, SLOT(placeOnRight()));
	connect(actionShow_Hide_Console, SIGNAL(triggered()), this, SLOT(toggleConsoleVisibility()));
	connect(actionGo_to_Preview, SIGNAL(triggered()), this, SLOT(goToPreview()));
	
	connect(this, SIGNAL(destroyed()), qApp, SLOT(updateWindowMenus()));

	connect(actionPreferences, SIGNAL(triggered()), qApp, SLOT(preferences()));

	connect(menuEdit, SIGNAL(aboutToShow()), this, SLOT(editMenuAboutToShow()));

#ifdef Q_WS_MAC
	textEdit->installEventFilter(CmdKeyFilter::filter());
#endif

	connect(inputLine, SIGNAL(returnPressed()), this, SLOT(acceptInputLine()));

	QSETTINGS_OBJECT(settings);
	TWUtils::applyToolbarOptions(this, settings.value("toolBarIconSize", 2).toInt(), settings.value("toolBarShowText", false).toBool());

	QFont font = textEdit->font();
	if (settings.contains("font")) {
		QString fontString = settings.value("font").toString();
		if (fontString != "") {
			font.fromString(fontString);
			textEdit->setFont(font);
		}
	}
	font.setPointSize(font.pointSize() - 1);
	inputLine->setFont(font);
	inputLine->setLayoutDirection(Qt::LeftToRight);
	textEdit_console->setFont(font);
	textEdit_console->setLayoutDirection(Qt::LeftToRight);
	
	bool b = settings.value("wrapLines", true).toBool();
	actionWrap_Lines->setChecked(b);
	setWrapLines(b);

	b = settings.value("lineNumbers", false).toBool();
	actionLine_Numbers->setChecked(b);
	setLineNumbers(b);
	
	highlighter = new TeXHighlighter(textEdit->document(), this);
	connect(textEdit, SIGNAL(rehighlight()), highlighter, SLOT(rehighlight()));

	QString syntaxOption = settings.value("syntaxColoring").toString();
	QStringList options = TeXHighlighter::syntaxOptions();

	QSignalMapper *syntaxMapper = new QSignalMapper(this);
	connect(syntaxMapper, SIGNAL(mapped(int)), this, SLOT(setSyntaxColoring(int)));
	syntaxMapper->setMapping(actionSyntaxColoring_None, -1);
	connect(actionSyntaxColoring_None, SIGNAL(triggered()), syntaxMapper, SLOT(map()));

	QActionGroup *syntaxGroup = new QActionGroup(this);
	syntaxGroup->addAction(actionSyntaxColoring_None);

	int index = 0;
	foreach (const QString& opt, options) {
		QAction *action = menuSyntax_Coloring->addAction(opt, syntaxMapper, SLOT(map()));
		action->setCheckable(true);
		syntaxGroup->addAction(action);
		syntaxMapper->setMapping(action, index);
		if (opt == syntaxOption) {
			action->setChecked(true);
			setSyntaxColoring(index);
		}
		++index;
	}
	
	// kDefault_TabWidth is defined in PrefsDialog.h
	textEdit->setTabStopWidth(settings.value("tabWidth", kDefault_TabWidth).toInt());
	
	QString indentOption = settings.value("autoIndent").toString();
	options = CompletingEdit::autoIndentModes();
	
	QSignalMapper *indentMapper = new QSignalMapper(this);
	connect(indentMapper, SIGNAL(mapped(int)), textEdit, SLOT(setAutoIndentMode(int)));
	indentMapper->setMapping(actionAutoIndent_None, -1);
	connect(actionAutoIndent_None, SIGNAL(triggered()), indentMapper, SLOT(map()));
	
	QActionGroup *indentGroup = new QActionGroup(this);
	indentGroup->addAction(actionAutoIndent_None);
	
	index = 0;
	foreach (const QString& opt, options) {
		QAction *action = menuAuto_indent_Mode->addAction(opt, indentMapper, SLOT(map()));
		action->setCheckable(true);
		indentGroup->addAction(action);
		indentMapper->setMapping(action, index);
		if (opt == indentOption) {
			action->setChecked(true);
			textEdit->setAutoIndentMode(index);
		}
		++index;
	}

	QString quotesOption = settings.value("smartQuotes").toString();
	options = CompletingEdit::smartQuotesModes();

	QSignalMapper *quotesMapper = new QSignalMapper(this);
	connect(quotesMapper, SIGNAL(mapped(int)), textEdit, SLOT(setSmartQuotesMode(int)));
	quotesMapper->setMapping(actionSmartQuotes_None, -1);
	connect(actionSmartQuotes_None, SIGNAL(triggered()), quotesMapper, SLOT(map()));

	QActionGroup *quotesGroup = new QActionGroup(this);
	quotesGroup->addAction(actionSmartQuotes_None);

	menuSmart_Quotes_Mode->removeAction(actionApply_to_Selection);
	index = 0;
	foreach (const QString& opt, options) {
		QAction *action = menuSmart_Quotes_Mode->addAction(opt, quotesMapper, SLOT(map()));
		action->setCheckable(true);
		quotesGroup->addAction(action);
		quotesMapper->setMapping(action, index);
		if (opt == quotesOption) {
			action->setChecked(true);
			textEdit->setSmartQuotesMode(index);
		}
		++index;
	}
	if (options.size() > 0)
		menuSmart_Quotes_Mode->addSeparator();
	menuSmart_Quotes_Mode->addAction(actionApply_to_Selection);
	connect(actionApply_to_Selection, SIGNAL(triggered()), textEdit, SLOT(smartenQuotes()));

	connect(actionLine_Numbers, SIGNAL(triggered(bool)), this, SLOT(setLineNumbers(bool)));
	connect(actionWrap_Lines, SIGNAL(triggered(bool)), this, SLOT(setWrapLines(bool)));

	QSignalMapper *mapper = new QSignalMapper(this);
	connect(actionNone, SIGNAL(triggered()), mapper, SLOT(map()));
	mapper->setMapping(actionNone, QString());
	connect(mapper, SIGNAL(mapped(const QString&)), this, SLOT(setLangInternal(const QString&)));

	QActionGroup *group = new QActionGroup(this);
	group->addAction(actionNone);

	QString defLang = settings.value("language", tr("None")).toString();
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
	menuShow->addSeparator();

	TWUtils::zoomToHalfScreen(this);

	QDockWidget *dw = new TagsDock(this);
	dw->hide();
	addDockWidget(Qt::LeftDockWidgetArea, dw);
	menuShow->addAction(dw->toggleViewAction());
	deferTagListChanges = false;

	watcher = new QFileSystemWatcher(this);
	connect(watcher, SIGNAL(fileChanged(const QString&)), this, SLOT(reloadIfChangedOnDisk()));
	connect(watcher, SIGNAL(directoryChanged(const QString&)), this, SLOT(reloadIfChangedOnDisk()));
	
	docList.append(this);
	
	TWApp::instance()->updateWindowMenus();
	
	initScriptable(menuScripts, actionAbout_Scripts, actionManage_Scripts,
				   actionUpdate_Scripts, actionShow_Scripts_Folder);

	TWUtils::insertHelpMenuItems(menuHelp);
	TWUtils::installCustomShortcuts(this);
}

void TeXDocument::changeEvent(QEvent *event)
{
	if (event->type() == QEvent::LanguageChange) {
		QString title = windowTitle();
		retranslateUi(this);
		menuRecent->setTitle(tr("Open Recent"));
		TWUtils::insertHelpMenuItems(menuHelp);
		setWindowTitle(title);
		showCursorPosition();
	}
	else
		QMainWindow::changeEvent(event);
}

void TeXDocument::setLangInternal(const QString& lang)
{
	// called internally by the spelling menu actions;
	// not for use from scripts as it won't update the menu
	QTextCodec *spellingCodec;
	pHunspell = TWUtils::getDictionary(lang);
	if (pHunspell != NULL) {
		spellingCodec = QTextCodec::codecForName(Hunspell_get_dic_encoding(pHunspell));
		if (spellingCodec == NULL)
			spellingCodec = QTextCodec::codecForLocale(); // almost certainly wrong, if we couldn't find the actual name!
	}
	else
		spellingCodec = NULL;
	textEdit->setSpellChecker(pHunspell, spellingCodec);
	highlighter->setSpellChecker(pHunspell, spellingCodec);
}

void TeXDocument::setSpellcheckLanguage(const QString& lang)
{
	// this is called by the %!TEX spellcheck... line, or by scripts;
	// it searches the menu for the given language code, and triggers it if available
	if (menuSpelling) {
		QAction *chosen = menuSpelling->actions()[0]; // default is None
		foreach (QAction *act, menuSpelling->actions()) {
			if (act->text() == lang) {
				chosen = act;
				break;
			}
		}
		chosen->trigger();
	}
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
	doc->textEdit->updateLineNumberAreaWidth(0);
	runHooks("NewFile");
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
			doc->textEdit->updateLineNumberAreaWidth(0);
			doc->runHooks("NewFromTemplate");
		}
	}
}

void TeXDocument::makeUntitled()
{
	setCurrentFile("");
	actionRemove_Aux_Files->setEnabled(false);
}

void TeXDocument::open()
{
	QFileDialog::Options options = 0;
#ifdef Q_WS_MAC
		/* use a sheet if we're calling Open from an empty, untitled, untouched window; otherwise use a separate dialog */
	if (!(isUntitled && textEdit->document()->isEmpty() && !isWindowModified()))
		options = QFileDialog::DontUseSheet;
#endif
	QSETTINGS_OBJECT(settings);
	QString lastOpenDir = settings.value("openDialogDir").toString();
	if (lastOpenDir.isEmpty())
		lastOpenDir = QDir::homePath();
	QStringList files = QFileDialog::getOpenFileNames(this, QString(tr("Open File")), lastOpenDir, TWUtils::filterList()->join(";;"), NULL, options);
	foreach (QString fileName, files) {
		if (!fileName.isEmpty()) {
			QFileInfo info(fileName);
			settings.setValue("openDialogDir", info.canonicalPath());
			TWApp::instance()->openFile(fileName); // not TeXDocument::open() - give the app a chance to open as PDF
		}
	}
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

TeXDocument* TeXDocument::openDocument(const QString &fileName, bool activate, bool raiseWindow, int lineNo, int selStart, int selEnd) // static
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
				doc = NULL;
			}
		}
	}
	if (doc != NULL) {
		if (activate)
			doc->selectWindow();
		else {
			doc->show();
			if (raiseWindow) {
				doc->raise();
				if (doc->isMinimized())
					doc->showNormal();
			}
		}
		if (lineNo > 0)
			doc->goToLine(lineNo, selStart, selEnd);
	}
	return doc;
}

void TeXDocument::closeEvent(QCloseEvent *event)
{
/*
	if (process != NULL) {
		statusBar()->showMessage(tr("Cannot close window while tool is running"), kStatusMessageDuration);
		event->ignore();
		return;
	}
*/
	if (maybeSave()) {
		event->accept();
		deleteLater();
	}
	else
		event->ignore();
}

void TeXDocument::hideFloatersUnlessThis(QWidget* currWindow)
{
	TeXDocument* p = qobject_cast<TeXDocument*>(currWindow);
	if (p == this)
		return;
	foreach (QObject* child, children()) {
		QToolBar* tb = qobject_cast<QToolBar*>(child);
		if (tb && tb->isVisible() && tb->isFloating()) {
			latentVisibleWidgets.append(tb);
			tb->hide();
			continue;
		}
		QDockWidget* dw = qobject_cast<QDockWidget*>(child);
		if (dw && dw->isVisible() && dw->isFloating()) {
			latentVisibleWidgets.append(dw);
			dw->hide();
			continue;
		}
	}
}

void TeXDocument::showFloaters()
{
	foreach (QWidget* w, latentVisibleWidgets)
		w->show();
	latentVisibleWidgets.clear();
}

bool TeXDocument::event(QEvent *event) // based on example at doc.trolltech.com/qq/qq18-macfeatures.html
{
	switch (event->type()) {
		case QEvent::IconDrag:
			if (isActiveWindow()) {
				event->accept();
				Qt::KeyboardModifiers mods = qApp->keyboardModifiers();
				if (mods == Qt::NoModifier) {
					QDrag *drag = new QDrag(this);
					QMimeData *data = new QMimeData();
					data->setUrls(QList<QUrl>() << QUrl::fromLocalFile(curFile));
					drag->setMimeData(data);
					QPixmap dragIcon(":/images/images/TeXworks-doc-48.png");
					drag->setPixmap(dragIcon);
					drag->setHotSpot(QPoint(dragIcon.width() - 5, 5));
					drag->start(Qt::LinkAction | Qt::CopyAction);
				}
				else if (mods == Qt::ShiftModifier) {
					QMenu menu(this);
					connect(&menu, SIGNAL(triggered(QAction*)), this, SLOT(openAt(QAction*)));
					QFileInfo info(curFile);
					QAction *action = menu.addAction(info.fileName());
					action->setIcon(QIcon(":/images/images/TeXworks-doc.png"));
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
#ifdef Q_WS_MAC
					extern void qt_mac_set_menubar_icons(bool);
					qt_mac_set_menubar_icons(true);
#endif
					menu.exec(pos);
#ifdef Q_WS_MAC
					qt_mac_set_menubar_icons(false);
#endif
				}
				else {
					event->ignore();
				}
				return true;
			}

		case QEvent::WindowActivate:
			showFloaters();
			emit activatedWindow(this);
			break;

		default:
			break;
	}
	return QMainWindow::event(event);
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

bool TeXDocument::saveAll()
{
	bool savedAll = true;
	foreach (TeXDocument* doc, docList) {
		if (doc->textEdit->document()->isModified()) {
			if (!doc->save()) {
				savedAll = false;
			}
		}
	}
	return savedAll;
}

bool TeXDocument::saveAs()
{
#ifdef Q_WS_WIN
	QFileDialog::Options	options = QFileDialog::DontUseNativeDialog;
#else
	QFileDialog::Options	options = 0;
#endif
	QString selectedFilter;

	// for untitled docs, default to the last dir used, or $HOME if no saved value
	QSETTINGS_OBJECT(settings);
	QString lastSaveDir = settings.value("saveDialogDir").toString();
	if (lastSaveDir.isEmpty())
		lastSaveDir = QDir::homePath();
	QString fileName = QFileDialog::getSaveFileName(this, tr("Save File"),
													isUntitled ? lastSaveDir + "/" + curFile : curFile,
													TWUtils::filterList()->join(";;"),
													&selectedFilter, options);
	if (fileName.isEmpty())
		return false;

	// add extension from the selected filter, if unique and not already present
	QRegExp re("\\(\\*(\\.[^ *]+)\\)");
	if (re.indexIn(selectedFilter) >= 0) {
		QString ext = re.cap(1);
		if (!fileName.endsWith(ext, Qt::CaseInsensitive) && !fileName.endsWith("."))
			fileName.append(ext);
	}
	
	if (fileName != curFile) {
		// The pdf connection is no longer (necessarily) valid. Detach it for
		// now (the correct connection will be reestablished on next typeset).
		detachPdf();
	}

	QFileInfo info(fileName);
	settings.setValue("saveDialogDir", info.canonicalPath());
	
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

bool TeXDocument::saveFilesHavingRoot(const QString& aRootFile)
{
	foreach (TeXDocument* doc, docList) {
		if (doc->getRootFilePath() == aRootFile) {
			if (doc->textEdit->document()->isModified() && !doc->save())
				return false;
		}
	}
	return true;
}

const QString& TeXDocument::getRootFilePath()
{
	findRootFilePath();
	return rootFilePath;
}

void TeXDocument::revert()
{
	if (!isUntitled) {
		QMessageBox	messageBox(QMessageBox::Warning, tr(TEXWORKS_NAME),
					tr("Do you want to discard all changes to the document \"%1\", and revert to the last saved version?")
					   .arg(TWUtils::strippedName(curFile)), QMessageBox::Cancel, this);
		QAbstractButton *revertButton = messageBox.addButton(tr("Revert"), QMessageBox::DestructiveRole);
		messageBox.setDefaultButton(QMessageBox::Cancel);
		messageBox.exec();
		if (messageBox.clickedButton() == revertButton)
			loadFile(curFile);
	}
}

void TeXDocument::maybeEnableSaveAndRevert(bool modified)
{
	actionSave->setEnabled(modified || isUntitled);
	actionRevert_to_Saved->setEnabled(modified && !isUntitled);
}

static const char* texshopSynonyms[] = {
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
	QRegExp re("% *!TEX +encoding *= *([^\\r\\n\\x2029]+)[\\r\\n\\x2029]", Qt::CaseInsensitive);
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

QString TeXDocument::readFile(const QString &fileName,
							  QTextCodec **codecUsed,
							  int *lineEndings)
	// reads the text from a file, after checking for %!TEX encoding.... metadata
	// sets codecUsed to the QTextCodec used to read the text
	// returns a null (not just empty) QString on failure
{
	if (lineEndings != NULL) {
		// initialize to default for the platform
#ifdef Q_WS_WIN
		*lineEndings = kLineEnd_CRLF;
#else
		*lineEndings = kLineEnd_LF;
#endif
	}
	
	QFile file(fileName);
	// Not using QFile::Text because this prevents us reading "classic" Mac files
	// with CR-only line endings. See issue #242.
	if (!file.open(QFile::ReadOnly)) {
		QMessageBox::warning(this, tr(TEXWORKS_NAME),
							 tr("Cannot read file \"%1\":\n%2")
							 .arg(fileName)
							 .arg(file.errorString()));
		return QString();
	}

	QString peekStr(file.peek(PEEK_LENGTH));
	QString reqName;
	bool hasMetadata;
	*codecUsed = scanForEncoding(peekStr, hasMetadata, reqName);
	if (*codecUsed == NULL) {
		*codecUsed = TWApp::instance()->getDefaultCodec();
		if (hasMetadata) {
			if (QMessageBox::warning(this, tr("Unrecognized encoding"),
					tr("The text encoding %1 used in %2 is not supported.\n\n"
					   "It will be interpreted as %3 instead, which may result in incorrect text.")
						.arg(reqName)
						.arg(fileName)
						.arg(QString::fromAscii((*codecUsed)->name())),
					QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Ok) == QMessageBox::Cancel)
				return QString();
		}
	}
	
	if (file.atEnd())
		return QString("");
	else {
		QTextStream in(&file);
		in.setCodec(*codecUsed);
		QString text = in.readAll();

		if (lineEndings != NULL) {
			if (text.contains("\r\n")) {
				text.replace("\r\n", "\n");
				*lineEndings = kLineEnd_CRLF;
			}
			else if (text.contains("\r") && !text.contains("\n")) {
				text.replace("\r", "\n");
				*lineEndings = kLineEnd_CR;
			}
			else
				*lineEndings = kLineEnd_LF;

			if (text.contains("\r")) {
				text.replace("\r", "\n");
				*lineEndings |= kLineEnd_Mixed;
			}
		}

		return text;
	}
}

void TeXDocument::loadFile(const QString &fileName, bool asTemplate, bool inBackground)
{
	QString fileContents = readFile(fileName, &codec, &lineEndings);
	showLineEndingSetting();
	showEncodingSetting();

	if (fileContents.isNull())
		return;

	QApplication::setOverrideCursor(Qt::WaitCursor);
	deferTagListChanges = true;
	tagListChanged = false;
	textEdit->setPlainText(fileContents);
	deferTagListChanges = false;
	if (tagListChanged)
		emit tagListUpdated();
	QApplication::restoreOverrideCursor();

	if (asTemplate) {
		lastModified = QDateTime();
	}
	else {
		setCurrentFile(fileName);
		if (!inBackground) {
			show(); // ensure window is shown before the PDF, if opening a new doc
			showPdfIfAvailable();
			selectWindow();
		}

		statusBar()->showMessage(tr("File \"%1\" loaded").arg(TWUtils::strippedName(curFile)),
								 kStatusMessageDuration);
		setupFileWatcher();
	}
	textEdit->updateLineNumberAreaWidth(0);
	maybeEnableSaveAndRevert(false);

	runHooks("LoadFile");
}

void TeXDocument::reloadIfChangedOnDisk()
{
	if (isUntitled || !lastModified.isValid())
		return;

	QDateTime fileModified = QFileInfo(curFile).lastModified();
	if (!fileModified.isValid() || fileModified == lastModified)
		return;

	clearFileWatcher(); // stop watching until next save or reload
	if (textEdit->document()->isModified()) {
		if (QMessageBox::warning(this, tr("File changed on disk"),
								 tr("%1 has been modified by another program.\n\n"
									"Do you want to discard your current changes, and reload the file from disk?")
								 .arg(curFile),
								 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel) {
			lastModified = QDateTime();	// invalidate the timestamp
			return;
		}
	}
	// user chose to discard, or there were no local changes
	// save the current cursor position
	QTextCursor cur;
	int oldSelStart, oldSelEnd, oldBlockStart, oldBlockEnd;
	int xPos = 0, yPos = 0;
	QString oldSel;

	// Store the selection (note that oldSelStart == oldSelEnd if there is
	// no selection)
	cur = textEdit->textCursor();
	oldSelStart = cur.selectionStart();
	oldSelEnd = cur.selectionEnd();
	oldSel = cur.selectedText();

	// Get the block number and the offset in the block of the start of the
	// selection
	cur.setPosition(oldSelStart);
	oldBlockStart = cur.blockNumber();
	oldSelStart -= cur.block().position();

	// Get the block number and the offset in the block of the end of the
	// selection
	cur.setPosition(oldSelEnd);
	oldBlockEnd = cur.blockNumber();
	oldSelEnd -= cur.block().position();

	// Get the values of the scroll bars so we can later restore the view
	if (textEdit->horizontalScrollBar())
		xPos = textEdit->horizontalScrollBar()->value();
	if (textEdit->verticalScrollBar())
		yPos = textEdit->verticalScrollBar()->value();

	// Reload the file from the disk
	loadFile(curFile, false, true);

	// restore the cursor position
	cur = textEdit->textCursor();

	// move the cursor to the beginning (this should actually be the case,
	// but one never knows)
	cur.setPosition(0);

	// move the cursor to the starting block
	cur.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor, oldBlockStart);
	cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::MoveAnchor);
	cur.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, oldSelStart);
	
	// move the cursor to the end block
	cur.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor, oldBlockEnd - oldBlockStart);
	cur.movePosition(QTextCursor::StartOfBlock, QTextCursor::KeepAnchor);
	cur.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, oldSelEnd);
	
	// if the current selection doesn't match the stored selection, collapse
	// to the beginning position
	if (cur.selectedText() != oldSel)
		cur.setPosition(cur.selectionStart());

	textEdit->setTextCursor(cur);

	// restore the view
	if (textEdit->horizontalScrollBar())
		textEdit->horizontalScrollBar()->setValue(xPos);
	if (textEdit->verticalScrollBar())
		textEdit->verticalScrollBar()->setValue(yPos);
}

// get expected name of the Preview file, and return whether it exists
bool TeXDocument::getPreviewFileName(QString &pdfName)
{
	findRootFilePath();
	if (rootFilePath == "")
		return false;
	QFileInfo fi(rootFilePath);
	pdfName = fi.canonicalPath() + "/" + fi.completeBaseName() + ".pdf";
	fi.setFile(pdfName);
	return fi.exists();
}

bool TeXDocument::showPdfIfAvailable()
{
	detachPdf();
	actionSide_by_Side->setEnabled(false);
	actionGo_to_Preview->setEnabled(false);

	QString pdfName;
	if (getPreviewFileName(pdfName)) {
		PDFDocument *existingPdf = PDFDocument::findDocument(pdfName);
		if (existingPdf != NULL) {
			pdfDoc = existingPdf;
			pdfDoc->reload();
			pdfDoc->selectWindow();
			pdfDoc->linkToSource(this);
		}
		else {
			pdfDoc = new PDFDocument(pdfName, this);
			TWUtils::sideBySide(this, pdfDoc);
			pdfDoc->show();
		}
	}

	if (pdfDoc != NULL) {
		actionSide_by_Side->setEnabled(true);
		actionGo_to_Preview->setEnabled(true);
		connect(pdfDoc, SIGNAL(destroyed()), this, SLOT(pdfClosed()));
		connect(this, SIGNAL(destroyed(QObject*)), pdfDoc, SLOT(texClosed(QObject*)));
		return true;
	}
	
	return false;
}

void TeXDocument::pdfClosed()
{
	pdfDoc = NULL;
	actionSide_by_Side->setEnabled(false);
}

bool TeXDocument::saveFile(const QString &fileName)
{
	QFileInfo fileInfo(fileName);
	QDateTime fileModified = fileInfo.lastModified();
	if (fileName == curFile && fileModified.isValid() && fileModified != lastModified) {
		if (QMessageBox::warning(this, tr("File changed on disk"),
								 tr("%1 has been modified by another program.\n\n"
									"Do you want to proceed with saving this file, overwriting the version on disk?")
								 .arg(fileName),
								 QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel) {
			notSaved:
				statusBar()->showMessage(tr("Document \"%1\" was not saved")
										 .arg(TWUtils::strippedName(curFile)),
										 kStatusMessageDuration);
				return false;
		}
	}
	
	QString theText = textEdit->toPlainText();
	switch (lineEndings & kLineEnd_Mask) {
		case kLineEnd_CR:
			theText.replace("\n", "\r");
			break;
		case kLineEnd_LF:
			break;
		case kLineEnd_CRLF:
			theText.replace("\n", "\r\n");
			break;
	}
	
	if (!codec)
		codec = TWApp::instance()->getDefaultCodec();
	if (!codec->canEncode(theText)) {
		if (QMessageBox::warning(this, tr("Text cannot be converted"),
				tr("This document contains characters that cannot be represented in the encoding %1.\n\n"
				   "If you proceed, they will be replaced with default codes. "
				   "Alternatively, you may wish to use a different encoding (such as UTF-8) to avoid loss of data.")
					.arg(QString(codec->name())),
				QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel) == QMessageBox::Cancel)
			goto notSaved;
	}

	clearFileWatcher();

	{
		QFile file(fileName);
		if (!file.open(QFile::WriteOnly)) {
			QMessageBox::warning(this, tr(TEXWORKS_NAME),
								 tr("Cannot write file \"%1\":\n%2")
								 .arg(fileName)
								 .arg(file.errorString()));
			setupFileWatcher();
			goto notSaved;
		}

		QApplication::setOverrideCursor(Qt::WaitCursor);
		if (file.write(codec->fromUnicode(theText)) == -1) {
			QApplication::restoreOverrideCursor();
			QMessageBox::warning(this, tr("Error writing file"),
								 tr("An error may have occurred while saving the file. "
									"You might like to save a copy in a different location."),
								 QMessageBox::Ok);
			goto notSaved;
		}
		QApplication::restoreOverrideCursor();
	}

	setCurrentFile(fileName);
	statusBar()->showMessage(tr("File \"%1\" saved")
								.arg(TWUtils::strippedName(curFile)),
								kStatusMessageDuration);
	
	QTimer::singleShot(0, this, SLOT(setupFileWatcher()));
	return true;
}

void TeXDocument::clearFileWatcher()
{
	const QStringList files = watcher->files();
	if (files.count() > 0)
		watcher->removePaths(files);	
	const QStringList dirs = watcher->directories();
	if (dirs.count() > 0)
		watcher->removePaths(dirs);	
}

void TeXDocument::setupFileWatcher()
{
	clearFileWatcher();
	if (!isUntitled) {
		QFileInfo info(curFile);
		lastModified = info.lastModified();
		watcher->addPath(curFile);
		watcher->addPath(info.canonicalPath());
	}
}	

void TeXDocument::setCurrentFile(const QString &fileName)
{
	static int sequenceNumber = 1;

	curFile = QFileInfo(fileName).canonicalFilePath();
	isUntitled = curFile.isEmpty();
	if (isUntitled) {
		curFile = tr("untitled-%1.tex").arg(sequenceNumber++);
		setWindowIcon(QIcon());
	}
	else
		setWindowIcon(QIcon(":/images/images/TeXworks-doc.png"));

	textEdit->document()->setModified(false);
	setWindowModified(false);

	setWindowTitle(tr("%1[*] - %2").arg(TWUtils::strippedName(curFile)).arg(tr(TEXWORKS_NAME)));

	if (!isUntitled)
		TWApp::instance()->addToRecentFiles(curFile);
	
	actionRemove_Aux_Files->setEnabled(!isUntitled);
	
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
	engine->disconnect(this);
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
	}
	connect(engine, SIGNAL(currentIndexChanged(const QString&)), this, SLOT(selectedEngine(const QString&)));
	int index = engine->findText(engineName, Qt::MatchFixedString);
	if (index < 0)
		index = engine->findText(TWApp::instance()->getDefaultEngine().name(), Qt::MatchFixedString);
	if (index >= 0)
		engine->setCurrentIndex(index);
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
	QTextCursor cursor = textEdit->textCursor();
	cursor.setPosition(cursor.selectionStart());
	int line = cursor.blockNumber() + 1;
	int total = textEdit->document()->blockCount();
	int col = cursor.position() - textEdit->document()->findBlock(cursor.selectionStart()).position();
	lineNumberLabel->setText(tr("Line %1 of %2; col %3").arg(line).arg(total).arg(col));
	if (actionAuto_Follow_Focus->isChecked())
		emit syncFromSource(curFile, line, false);
}

void TeXDocument::showLineEndingSetting()
{
	QString lineEndStr;
	switch (lineEndings & kLineEnd_Mask) {
		case kLineEnd_LF:
			lineEndStr = "LF";
			break;
		case kLineEnd_CRLF:
			lineEndStr = "CRLF";
			break;
		case kLineEnd_CR:
			lineEndStr = "CR";
			break;
	}
	if ((lineEndings & kLineEnd_Mixed) != 0)
		lineEndStr += "*";
	lineEndingLabel->setText(lineEndStr);
}

void TeXDocument::lineEndingPopup(const QPoint loc)
{
	QMenu menu;
	QAction *cr, *lf, *crlf;
	menu.addAction(lf = new QAction("LF (Unix, Mac OS X)", &menu));
	menu.addAction(crlf = new QAction("CRLF (Windows)", &menu));
	menu.addAction(cr = new QAction("CR (Mac Classic)", &menu));
	QAction *result = menu.exec(lineEndingLabel->mapToGlobal(loc));
	int newSetting = (lineEndings & kLineEnd_Mask);
	if (result == lf)
		newSetting = kLineEnd_LF;
	else if (result == crlf)
		newSetting = kLineEnd_CRLF;
	else if (result == cr)
		newSetting = kLineEnd_CR;
	if (newSetting != (lineEndings & kLineEnd_Mask)) {
		lineEndings = newSetting;
		showLineEndingSetting();
		textEdit->document()->setModified();
	}
}

void TeXDocument::showEncodingSetting()
{
	encodingLabel->setText(codec ? codec->name() : "");
}

void TeXDocument::encodingPopup(const QPoint loc)
{
	QMenu menu;
	foreach (QTextCodec *codec, *TWUtils::findCodecs())
		menu.addAction(new QAction(codec->name(), &menu));
	QAction *result = menu.exec(encodingLabel->mapToGlobal(loc));
	if (result) {
		QTextCodec *newCodec = QTextCodec::codecForName(result->text().toAscii());
		if (newCodec && newCodec != codec) {
			codec = newCodec;
			showEncodingSetting();
			textEdit->document()->setModified();
		}
	}
}

void TeXDocument::selectWindow(bool activate)
{
	show();
	raise();
	if (activate)
		activateWindow();
	if (isMinimized())
		showNormal();
}

void TeXDocument::sideBySide()
{
	if (pdfDoc != NULL) {
		TWUtils::sideBySide(this, pdfDoc);
		pdfDoc->selectWindow(false);
		selectWindow();
	}
	else
		placeOnLeft();
}

void TeXDocument::placeOnLeft()
{
	TWUtils::zoomToHalfScreen(this, false);
}

void TeXDocument::placeOnRight()
{
	TWUtils::zoomToHalfScreen(this, true);
}

TeXDocument *TeXDocument::findDocument(const QString &fileName)
{
	QString canonicalFilePath = QFileInfo(fileName).canonicalFilePath();
	if (canonicalFilePath.isEmpty())
		canonicalFilePath = fileName;
			// file doesn't exist (probably from find-results in a new untitled doc),
			// so just use the name as-is

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

QString TeXDocument::getLineText(int lineNo) const
{
	QTextDocument* doc = textEdit->document();
	if (lineNo < 1 || lineNo > doc->blockCount())
		return QString();
#if QT_VERSION >= 0x040400
	return doc->findBlockByNumber(lineNo - 1).text();
#else
	QTextBlock block = doc->findBlock(0);
	while (--lineNo > 0)
		block = block.next();
	return block.text();
#endif
}

void TeXDocument::goToLine(int lineNo, int selStart, int selEnd)
{
	QTextDocument* doc = textEdit->document();
	if (lineNo < 1 || lineNo > doc->blockCount())
		return;
	int oldScrollValue = -1;
	if (textEdit->verticalScrollBar() != NULL)
		oldScrollValue = textEdit->verticalScrollBar()->value();
#if QT_VERSION >= 0x040400
	QTextCursor cursor(doc->findBlockByNumber(lineNo - 1));
#else
	QTextBlock block = doc->findBlock(0);
	while (--lineNo > 0)
		block = block.next();
	QTextCursor cursor(block);
#endif
	if (selStart >= 0 && selEnd >= selStart) {
		cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, selStart);
		cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, selEnd - selStart);
	}
	else
		cursor.movePosition(QTextCursor::NextBlock, QTextCursor::KeepAnchor);
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
	bool ok;
	QFont font = QFontDialog::getFont(&ok, textEdit->font());
	if (ok) {
		textEdit->setFont(font);
		font.setPointSize(font.pointSize() - 1);
		textEdit_console->setFont(font);
		inputLine->setFont(font);
	}
}

void TeXDocument::doLineDialog()
{
	QTextCursor cursor = textEdit->textCursor();
	cursor.setPosition(cursor.selectionStart());
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
		doFindAgain(true);
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

void TeXDocument::toUppercase()
{
	replaceSelection(textEdit->textCursor().selectedText().toUpper());
}

void TeXDocument::toLowercase()
{
	replaceSelection(textEdit->textCursor().selectedText().toLower());
}

void TeXDocument::toggleCase()
{
	QString theText = textEdit->textCursor().selectedText();
	for (int i = 0; i < theText.length(); ++i) {
		QCharRef ch = theText[i];
		if (ch.isLower())
			ch = ch.toUpper();
		else
			ch = ch.toLower();
	}
	replaceSelection(theText);
}

void TeXDocument::replaceSelection(const QString& newText)
{
	QTextCursor cursor = textEdit->textCursor();
	int start = cursor.selectionStart();
	cursor.insertText(newText);
	int end = cursor.selectionEnd();
	cursor.setPosition(start);
	cursor.setPosition(end, QTextCursor::KeepAnchor);
	textEdit->setTextCursor(cursor);
}

void TeXDocument::selectRange(int start, int length)
{
	QTextCursor c = textCursor();
	c.setPosition(start);
	c.setPosition(start + length, QTextCursor::KeepAnchor);
	editor()->setTextCursor(c);
}

void TeXDocument::insertText(const QString& text)
{
	textCursor().insertText(text);
}

void TeXDocument::balanceDelimiters()
{
	const QString text = textEdit->toPlainText();
	QTextCursor cursor = textEdit->textCursor();
	int openPos = TWUtils::findOpeningDelim(text, cursor.selectionStart());
	if (openPos >= 0 && openPos < text.length() - 1) {
		do {
			int closePos = TWUtils::balanceDelim(text, openPos + 1, TWUtils::closerMatching(text[openPos]), 1);
			if (closePos < 0)
				break;
			if (closePos >= cursor.selectionEnd()) {
				cursor.setPosition(openPos);
				cursor.setPosition(closePos + 1, QTextCursor::KeepAnchor);
				textEdit->setTextCursor(cursor);
				return;
			}
			if (openPos > 0)
				openPos = TWUtils::findOpeningDelim(text, openPos - 1);
			else
				break;
		} while (openPos >= 0);
	}
	QApplication::beep();
}

void TeXDocument::doHardWrapDialog()
{
	HardWrapDialog dlg(this);
	
	dlg.show();
	if (dlg.exec()) {
		dlg.saveSettings();
		doHardWrap(dlg.lineWidth(), dlg.rewrap());
	}
}

void TeXDocument::doHardWrap(int lineWidth, bool rewrap)
{
	if (lineWidth == 0) { // use window width (approx)
		// fudge this for now.... not accurate with proportional fonts, ignores tabs,....
		QFontMetrics fm(textEdit->currentFont());
		lineWidth = textEdit->width() / fm.averageCharWidth();
	}
	
	QTextCursor cur = textEdit->textCursor();
	if (!cur.hasSelection())
		cur.select(QTextCursor::Document);
		
	int selStart = cur.selectionStart();
	int selEnd = cur.selectionEnd();

	cur.setPosition(selStart);
	if (!cur.atBlockStart()) {
		cur.movePosition(QTextCursor::StartOfBlock);
		selStart = cur.position();
	}
	
	cur.setPosition(selEnd);
	if (!cur.atBlockStart()) {
		cur.movePosition(QTextCursor::NextBlock);
		selEnd = cur.position();
	}

	cur.setPosition(selStart);
	cur.setPosition(selEnd, QTextCursor::KeepAnchor);
	
	QString oldText = cur.selectedText();
	QRegExp breakPattern("\\s+");
	QString newText;
	
	while (!oldText.isEmpty()) {
		int eol = oldText.indexOf(QChar::ParagraphSeparator);
		if (eol == -1)
			eol = oldText.length();
		else
			eol += 1;
		QString line = oldText.left(eol);
		oldText.remove(0, eol);

		if (rewrap && line.trimmed().length() > 0) {
			while (!oldText.isEmpty()) {
				eol = oldText.indexOf(QChar::ParagraphSeparator);
				if (eol == -1)
					eol = oldText.length();
				QString nextLine = oldText.left(eol).trimmed();
				if (nextLine.isEmpty())
					break;
				line = line.trimmed().append(QChar(' ')).append(nextLine);
				oldText.remove(0, eol + 1);
			}
		}
		
		if (line.length() <= lineWidth) {
			newText.append(line);
			continue;
		}

		line = line.trimmed();
		if (line.length() <= lineWidth) {
			newText.append(line);
			continue;
		}

		int curLength = 0;
		while (!line.isEmpty()) {
			int breakPoint = line.indexOf(breakPattern);
			int matchLen = breakPattern.matchedLength();
			if (breakPoint == -1) {
				breakPoint = line.length();
				matchLen = 0;
			}
			if (curLength > 0 && curLength + breakPoint >= lineWidth) {
				newText.append(QChar::ParagraphSeparator);
				curLength = 0;
			}
			if (curLength > 0) {
				newText.append(QChar(' '));
				curLength += 1;
			}
			newText.append(line.left(breakPoint));
			curLength += breakPoint;
			line.remove(0, breakPoint + matchLen);
		}
		newText.append(QChar::ParagraphSeparator);
	}
	
	cur.insertText(newText);

	selEnd = cur.position();
	cur.setPosition(selStart);
	cur.setPosition(selEnd, QTextCursor::KeepAnchor);
	textEdit->setTextCursor(cur);
}


void TeXDocument::setLineNumbers(bool displayNumbers)
{
	textEdit->setLineNumberDisplay(displayNumbers);
}

void TeXDocument::setWrapLines(bool wrap)
{
	textEdit->setWordWrapMode(wrap ? QTextOption::WordWrap : QTextOption::NoWrap);
}

void TeXDocument::setSyntaxColoring(int index)
{
	highlighter->setActiveIndex(index);
}

void TeXDocument::doFindAgain(bool fromDialog)
{
	QSETTINGS_OBJECT(settings);
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

	if (fromDialog && (settings.value("searchFindAll").toBool() || settings.value("searchAllFiles").toBool())) {
		bool singleFile = true;
		QList<SearchResult> results;
		flags &= ~QTextDocument::FindBackward;
		int docListIndex = 0;
		TeXDocument* theDoc = this;
		while (1) {
			QTextCursor curs(theDoc->textDoc());
			curs.movePosition(QTextCursor::End);
			int rangeStart = 0;
			int rangeEnd = curs.position();
			while (1) {
				curs = doSearch(theDoc->textDoc(), searchText, regex, flags, rangeStart, rangeEnd);
				if (curs.isNull())
					break;
				int blockStart = curs.block().position();
				results.append(SearchResult(theDoc, curs.blockNumber() + 1,
								curs.selectionStart() - blockStart, curs.selectionEnd() - blockStart));
				if ((flags & QTextDocument::FindBackward) != 0)
					rangeEnd = curs.selectionStart();
				else
					rangeStart = curs.selectionEnd();
			}

			if (settings.value("searchAllFiles").toBool() == false)
				break;
			// go to next document
		next_doc:
			if (docList[docListIndex] == theDoc)
				docListIndex++;
			if (docListIndex == docList.count())
				break;
			theDoc = docList[docListIndex];
			if (theDoc == this)
				goto next_doc;
			singleFile = false;
		}
		
		if (results.count() == 0) {
			qApp->beep();
			statusBar()->showMessage(tr("Not found"), kStatusMessageDuration);
		}
		else {
			SearchResults::presentResults(searchText, results, this, singleFile);
			statusBar()->showMessage(tr("Found %n occurrence(s)", "", results.count()), kStatusMessageDuration);
		}
	}
	else {
		QTextCursor	curs = textEdit->textCursor();
		if (settings.value("searchSelection").toBool() && curs.hasSelection()) {
			int rangeStart = curs.selectionStart();
			int rangeEnd = curs.selectionEnd();
			curs = doSearch(textEdit->document(), searchText, regex, flags, rangeStart, rangeEnd);
		}
		else {
			if ((flags & QTextDocument::FindBackward) != 0) {
				int rangeStart = 0;
				int rangeEnd = curs.selectionStart();
				curs = doSearch(textEdit->document(), searchText, regex, flags, rangeStart, rangeEnd);
				if (curs.isNull() && settings.value("searchWrap").toBool()) {
					curs = QTextCursor(textEdit->document());
					curs.movePosition(QTextCursor::End);
					curs = doSearch(textEdit->document(), searchText, regex, flags, 0, curs.position());
				}
			}
			else {
				int rangeStart = curs.selectionEnd();
				curs.movePosition(QTextCursor::End);
				int rangeEnd = curs.position();
				curs = doSearch(textEdit->document(), searchText, regex, flags, rangeStart, rangeEnd);
				if (curs.isNull() && settings.value("searchWrap").toBool())
					curs = doSearch(textEdit->document(), searchText, regex, flags, 0, rangeEnd);
			}
		}

		if (curs.isNull()) {
			qApp->beep();
			statusBar()->showMessage(tr("Not found"), kStatusMessageDuration);
		}
		else
			textEdit->setTextCursor(curs);
	}

	if (regex != NULL)
		delete regex;
}

void TeXDocument::doReplaceAgain()
{
	doReplace(ReplaceDialog::ReplaceOne);
}

void TeXDocument::doReplace(ReplaceDialog::DialogCode mode)
{
	QSETTINGS_OBJECT(settings);
	
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

	QString	replacement = settings.value("replaceText").toString();
	if (regex != NULL) {
		QRegExp escapedChar("\\\\([nt\\\\]|x([0-9A-Fa-f]{4}))");
		int index = -1;
		while ((index = replacement.indexOf(escapedChar, index + 1)) >= 0) {
			QChar ch;
			if (escapedChar.cap(1).length() == 1) {
				// single-char escape code newline/tab/backslash
				ch = escapedChar.cap(1)[0];
				switch (ch.unicode()) {
					case 'n':
						ch = '\n';
						break;
					case 't':
						ch = '\t';
						break;
					case '\\':
						ch = '\\';
						break;
					default:
						// should not happen!
						break;
				}
			}
			else {
				// Unicode char number \xHHHH
				bool ok;
				ch = (QChar)escapedChar.cap(2).toUInt(&ok, 16);
			}
			replacement.replace(index, escapedChar.matchedLength(), ch);
		}
	}
	
	bool allFiles = (mode == ReplaceDialog::ReplaceAll) && settings.value("searchAllFiles").toBool();
	
	bool searchWrap = settings.value("searchWrap").toBool();
	bool searchSel = settings.value("searchSelection").toBool();
	
	int rangeStart, rangeEnd;
	QTextCursor searchRange = textCursor();
	if (allFiles) {
		searchRange.select(QTextCursor::Document);
		rangeStart = searchRange.selectionStart();
		rangeEnd = searchRange.selectionEnd();
	}
	else if (searchSel) {
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
		QTextCursor curs = doSearch(textEdit->document(), searchText, regex, flags, rangeStart, rangeEnd);
		if (curs.isNull()) {
			qApp->beep();
			statusBar()->showMessage(tr("Not found"), kStatusMessageDuration);
		}
		else {
			// do replacement
			QString target;
			if (regex != NULL)
				target = textEdit->document()->toPlainText()
							.mid(curs.selectionStart(), curs.selectionEnd() - curs.selectionStart()).replace(*regex, replacement);
			else
				target = replacement;
			curs.insertText(target);
			textEdit->setTextCursor(curs);
		}
	}
	else if (mode == ReplaceDialog::ReplaceAll) {
		if (allFiles) {
			int replacements = 0;
			foreach (TeXDocument* doc, docList)
				replacements += doc->doReplaceAll(searchText, regex, replacement, flags);
			QString numOccurrences = tr("%n occurrence(s)", "", replacements);
			QString numDocuments = tr("%n documents", "", docList.count());
			QString message = tr("Replaced %1 in %2").arg(numOccurrences).arg(numDocuments);
			statusBar()->showMessage(message, kStatusMessageDuration);
		}
		else {
			int replacements = doReplaceAll(searchText, regex, replacement, flags, rangeStart, rangeEnd);
			statusBar()->showMessage(tr("Replaced %n occurrence(s)", "", replacements), kStatusMessageDuration);
		}
	}

	if (regex != NULL)
		delete regex;
}

int TeXDocument::doReplaceAll(const QString& searchText, QRegExp* regex, const QString& replacement,
								QTextDocument::FindFlags flags, int rangeStart, int rangeEnd)
{
	QTextCursor searchRange = textCursor();
	searchRange.select(QTextCursor::Document);
	if (rangeStart < 0)
		rangeStart = searchRange.selectionStart();
	if (rangeEnd < 0)
		rangeEnd = searchRange.selectionEnd();
		
	int replacements = 0;
	bool first = true;
	while (1) {
		QTextCursor curs = doSearch(textEdit->document(), searchText, regex, flags, rangeStart, rangeEnd);
		if (curs.isNull()) {
			if (!first)
				searchRange.endEditBlock();
			break;
		}
		if (first) {
			searchRange.beginEditBlock();
			first = false;
		}
		QString target;
		int oldLen = curs.selectionEnd() - curs.selectionStart();
		if (regex != NULL)
			target = textEdit->document()->toPlainText().mid(curs.selectionStart(), oldLen).replace(*regex, replacement);
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
	if (!first) {
		searchRange.setPosition(rangeStart);
		textEdit->setTextCursor(searchRange);
	}
	return replacements;
}

QTextCursor TeXDocument::doSearch(QTextDocument *theDoc, const QString& searchText, const QRegExp *regex, QTextDocument::FindFlags flags, int s, int e)
{
	QTextCursor curs;
	const QString& docText = theDoc->toPlainText();
	
	if ((flags & QTextDocument::FindBackward) != 0) {
		if (regex != NULL) {
			// this doesn't seem to match \n or even \x2029 for newline
			// curs = theDoc->find(*regex, e, flags);
			int offset = regex->lastIndexIn(docText, e, QRegExp::CaretAtZero);
			while (offset >= s && offset + regex->matchedLength() > e)
				offset = regex->lastIndexIn(docText, offset - 1, QRegExp::CaretAtZero);
			if (offset >= s) {
				curs = QTextCursor(theDoc);
				curs.setPosition(offset);
				curs.setPosition(offset + regex->matchedLength(), QTextCursor::KeepAnchor);
			}
		}
		else {
			curs = theDoc->find(searchText, e, flags);
			if (!curs.isNull()) {
				if (curs.selectionEnd() > e)
					curs = theDoc->find(searchText, curs, flags);
				if (curs.selectionStart() < s)
					curs = QTextCursor();
			}
		}
	}
	else {
		if (regex != NULL) {
			// this doesn't seem to match \n or even \x2029 for newline
			// curs = theDoc->find(*regex, s, flags);
			int offset = regex->indexIn(docText, s, QRegExp::CaretAtZero);
			if (offset >= 0) {
				curs = QTextCursor(theDoc);
				curs.setPosition(offset);
				curs.setPosition(offset + regex->matchedLength(), QTextCursor::KeepAnchor);
			}
		}
		else {
			curs = theDoc->find(searchText, s, flags);
		}
		if (curs.selectionEnd() > e)
			curs = QTextCursor();
	}
	return curs;
}

void TeXDocument::copyToFind()
{
	if (textEdit->textCursor().hasSelection()) {
		QString searchText = textEdit->textCursor().selectedText();
		searchText.replace(QString(0x2029), "\n");
		QSETTINGS_OBJECT(settings);
		if (settings.value("searchRegex").toBool())
			searchText = QRegExp::escape(searchText);
		settings.setValue("searchText", searchText);
	}
}

void TeXDocument::copyToReplace()
{
	if (textEdit->textCursor().hasSelection()) {
		QString replaceText = textEdit->textCursor().selectedText();
		replaceText.replace(QString(0x2029), "\n");
		QSETTINGS_OBJECT(settings);
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
	if (!saveFilesHavingRoot(rootFilePath))
		return;

	QFileInfo fileInfo(rootFilePath);
	if (!fileInfo.isReadable()) {
		statusBar()->showMessage(tr("Root document %1 is not readable").arg(rootFilePath), kStatusMessageDuration);
		return;
	}

	Engine e = TWApp::instance()->getNamedEngine(engine->currentText());
	if (e.program() == "") {
		statusBar()->showMessage(tr("%1 is not properly configured").arg(engine->currentText()), kStatusMessageDuration);
		return;
	}

	process = new QProcess(this);
	updateTypesettingAction();

	QString workingDir = fileInfo.canonicalPath();	// Note that fileInfo refers to the root file
#ifdef Q_WS_WIN
	// files in the root directory of the current drive have to be handled specially
	// because QFileInfo::canonicalPath() returns a path without trailing slash
	// (i.e., a bare drive letter)
	if (workingDir.length() == 2 && workingDir.endsWith(':'))
		workingDir.append('/');
#endif
	process->setWorkingDirectory(workingDir);

#ifdef Q_WS_WIN
#define PATH_CASE_SENSITIVE	Qt::CaseInsensitive
#else
#define PATH_CASE_SENSITIVE	Qt::CaseSensitive
#endif
	QStringList binPaths = TWApp::instance()->getBinaryPaths();
	QStringList env = QProcess::systemEnvironment();
	QMutableStringListIterator envIter(env);
	while (envIter.hasNext()) {
		QString& envVar = envIter.next();
		if (envVar.startsWith("PATH=", PATH_CASE_SENSITIVE)) {
			foreach (const QString& s, envVar.mid(5).split(QChar(PATH_LIST_SEP), QString::SkipEmptyParts))
			if (!binPaths.contains(s))
				binPaths.append(s);
			envVar = envVar.left(5) + binPaths.join(QChar(PATH_LIST_SEP));
			break;
		}
	}
	
	bool foundCommand = false;
	QFileInfo exeFileInfo;
	QStringListIterator pathIter(binPaths);
#ifdef Q_WS_WIN
	QStringList executableTypes = QStringList() << "exe" << "com" << "cmd" << "bat";
#endif
	while (pathIter.hasNext() && !foundCommand) {
		QString path = pathIter.next();
		exeFileInfo = QFileInfo(path, e.program());
		foundCommand = exeFileInfo.exists() && exeFileInfo.isExecutable();
#ifdef Q_WS_WIN
		// try adding common executable extensions, if one was not already present
		if (!foundCommand && !executableTypes.contains(exeFileInfo.suffix())) {
			QStringListIterator extensions(executableTypes);
			while (extensions.hasNext() && !foundCommand) {
				exeFileInfo = QFileInfo(path, e.program() + "." + extensions.next());
				foundCommand = exeFileInfo.exists() && exeFileInfo.isExecutable();
			}
		}
#endif
	}
	
	if (foundCommand) {
		QStringList args = e.arguments();
		
		// for old MikTeX versions: delete $synctexoption if it causes an error
		static bool checkedForSynctex = false;
		static bool synctexSupported = true;
		if (!checkedForSynctex) {
			QStringListIterator pi(binPaths);
			QFileInfo chkFileInfo;
			bool found = false;
			while (pi.hasNext() && !found) {
				QString path = pi.next();
				chkFileInfo = QFileInfo(path, "pdftex" EXE);
				if (chkFileInfo.exists())
					found = true;
			}
			if (found) {
				int result = QProcess::execute(chkFileInfo.absoluteFilePath(), QStringList() << "-synctex=1" << "-version");
				synctexSupported = (result == 0);
			}
			checkedForSynctex = true;
		}
		if (!synctexSupported)
			args.removeAll("$synctexoption");
		
		args.replaceInStrings("$synctexoption", "-synctex=1");
		args.replaceInStrings("$fullname", fileInfo.fileName());
		args.replaceInStrings("$basename", fileInfo.completeBaseName());
		args.replaceInStrings("$suffix", fileInfo.suffix());
		args.replaceInStrings("$directory", fileInfo.absoluteDir().absolutePath());
		
		textEdit_console->clear();
		if (consoleTabs->isHidden()) {
			keepConsoleOpen = false;
			showConsole();
		}
		else {
			inputLine->show();
		}
		inputLine->setFocus(Qt::OtherFocusReason);
		showPdfWhenFinished = e.showPdf();
		userInterrupt = false;

		process->setEnvironment(env);
		process->setProcessChannelMode(QProcess::MergedChannels);
		
		connect(process, SIGNAL(readyReadStandardOutput()), this, SLOT(processStandardOutput()));
		connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(processError(QProcess::ProcessError)));
		connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), this, SLOT(processFinished(int, QProcess::ExitStatus)));
		
		QString pdfName;
		if (getPreviewFileName(pdfName))
			oldPdfTime = QFileInfo(pdfName).lastModified();
		else
			oldPdfTime = QDateTime();
		
		process->start(exeFileInfo.absoluteFilePath(), args);
	}
	else {
		process->deleteLater();
		process = NULL;
		QMessageBox::critical(this, tr("Unable to execute %1").arg(e.name()),
							  "<p>" + tr("The program \"%1\" was not found.").arg(e.program()) +
							  "<p><small>" + tr("Searched in directories:") +
							  "<ul><li>" + binPaths.join("<li>") + "</ul></small>" +
							  "<p>" + tr("Check configuration of the %1 tool and path settings in the Preferences dialog.").arg(e.name()),
							  QMessageBox::Cancel);
		updateTypesettingAction();
	}
}

void TeXDocument::interrupt()
{
	if (process != NULL) {
		userInterrupt = true;
		process->kill();
	}
}

void TeXDocument::updateTypesettingAction()
{
	if (process == NULL) {
		disconnect(actionTypeset, SIGNAL(triggered()), this, SLOT(interrupt()));
		actionTypeset->setIcon(QIcon(":/images/images/runtool.png"));
		actionTypeset->setText(tr("Typeset"));
		connect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));
		if (pdfDoc != NULL)
			pdfDoc->updateTypesettingAction(false);
	}
	else {
		disconnect(actionTypeset, SIGNAL(triggered()), this, SLOT(typeset()));
		actionTypeset->setIcon(QIcon(":/images/tango/process-stop.png"));
		actionTypeset->setText(tr("Abort typesetting"));
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
	if (userInterrupt)
		textEdit_console->append(tr("Process interrupted by user"));
	else
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
		QString pdfName;
		if (getPreviewFileName(pdfName) && QFileInfo(pdfName).lastModified() != oldPdfTime) {
			// only open/refresh the PDF if it was changed by the typeset process
			if (pdfDoc == NULL) {
				if (showPdfWhenFinished && showPdfIfAvailable())
					pdfDoc->selectWindow();
			}
			else {
				pdfDoc->reload(); // always reload if it is loaded, we don't want a stale window
				if (showPdfWhenFinished)
					pdfDoc->selectWindow();
			}
		}
	}

	executeAfterTypesetHooks();
	
	QSETTINGS_OBJECT(settings);
	if (!keepConsoleOpen && exitCode == 0 && exitStatus != QProcess::CrashExit && settings.value("autoHideConsole", true).toBool())
		hideConsole();
	else
		inputLine->hide();

	process->deleteLater();
	process = NULL;
	updateTypesettingAction();
}

void TeXDocument::executeAfterTypesetHooks()
{
	TWScriptManager * scriptManager = TWApp::instance()->getScriptManager();

	for (int i = consoleTabs->count() - 1; i > 0; --i)
		consoleTabs->removeTab(i);
	
	foreach (TWScript *s, scriptManager->getHookScripts("AfterTypeset")) {
		QVariant result;
		bool success = s->run(this, result);
		if (success && !result.isNull()) {
			if (result.type() == QVariant::List) {
				const QVariantList list = result.toList();
				int columns = 1;
				QTableWidget *table = new QTableWidget(list.count(), columns, this);
				table->horizontalHeader()->setResizeMode(QHeaderView::ResizeToContents);
				table->horizontalHeader()->setStretchLastSection(true);
				table->horizontalHeader()->hide();
				table->verticalHeader()->setResizeMode(QHeaderView::ResizeToContents);
				table->setSelectionBehavior(QAbstractItemView::SelectRows);
				table->setSelectionMode(QAbstractItemView::SingleSelection);
				table->setEditTriggers(QAbstractItemView::NoEditTriggers);
				connect(table, SIGNAL(itemDoubleClicked(QTableWidgetItem*)), this, SLOT(errorLineClicked(QTableWidgetItem*)));
				for (int i = 0; i < list.count(); ++i) {
					const QVariant item = list.at(i);
					if (item.type() == QVariant::List) {
						const QVariantList rowList = item.toList();
						if (rowList.count() > columns) {
							columns = rowList.count();
							table->setColumnCount(columns);
						}
						for (int j = 0; j < rowList.count(); ++j) {
							table->setItem(i, j, new QTableWidgetItem(rowList.at(j).toString()));
						}
					}
					else {
						table->setItem(i, 0, new QTableWidgetItem(item.toString()));
						table->setSpan(i, 0, 1, columns);
					}
				}
				consoleTabs->addTab(table, s->getTitle());
			}
			else {
				QTextEdit *textEdit = new QTextEdit(this);
				textEdit->setPlainText(result.toString());
				textEdit->setReadOnly(true);
				consoleTabs->addTab(textEdit, s->getTitle());
			}
		}
	}
}

void TeXDocument::errorLineClicked(QTableWidgetItem * i)
{
	QTableWidget * table = i->tableWidget();
	int row = i->row();
	QString filename = table->item(row, 0)->text();
	int line = table->item(row, 1)->text().toInt();
	
	openDocument(QFileInfo(curFile).absoluteDir().filePath(filename), true, true, line);
}

// showConsole() and hideConsole() are used internally to update the visibility;
// they must NOT change the keepConsoleOpen setting that records user choice
void TeXDocument::showConsole()
{
	consoleTabs->show();
	if (process != NULL)
		inputLine->show();
	actionShow_Hide_Console->setText(tr("Hide Output Panel"));
}

void TeXDocument::hideConsole()
{
	consoleTabs->hide();
	inputLine->hide();
	actionShow_Hide_Console->setText(tr("Show Output Panel"));
}

// this is connected to the user command, so remember the choice
// for when typesetting finishes
void TeXDocument::toggleConsoleVisibility()
{
	if (consoleTabs->isVisible()) {
		hideConsole();
		keepConsoleOpen = false;
	}
	else {
		showConsole();
		keepConsoleOpen = true;
	}
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
	else {
		if (!showPdfIfAvailable()) {
			// This should only fail if the user has done something sneaky like closing the
			// preview window and then renaming the PDF file, since we opened the source
			// and checked that it exists (otherwise Go to Preview would have been disabled).
			// We could issue a status-bar warning here but it's a pretty obscure case...
			// for now just disable the command.
			actionGo_to_Preview->setEnabled(false);
			actionSide_by_Side->setEnabled(false);
		}
	}
}

void TeXDocument::syncClick(int lineNo)
{
	if (!isUntitled) {
		// ensure that there is a pdf to receive our signal
		goToPreview();
		emit syncFromSource(curFile, lineNo, true);
	}
}

void TeXDocument::contentsChanged(int position, int /*charsRemoved*/, int /*charsAdded*/)
{
	if (position < PEEK_LENGTH) {
		int pos;
		QTextCursor curs(textEdit->document());
		curs.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, PEEK_LENGTH);
		QString peekStr = curs.selectedText();
		
		/* Search for engine specification */
		QRegExp re("% *!TEX +(?:TS-)?program *= *([^\\x2029]+)\\x2029", Qt::CaseInsensitive);
		pos = re.indexIn(peekStr);
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
		
		/* Search for encoding specification */
		bool hasMetadata;
		QString reqName;
		QTextCodec *newCodec = scanForEncoding(peekStr, hasMetadata, reqName);
		if (newCodec != NULL) {
			codec = newCodec;
			showEncodingSetting();
		}
		
		/* Search for spellcheck specification */
		QRegExp reSpell("% *!TEX +spellcheck *= *([^\\x2029]+)\\x2029", Qt::CaseInsensitive);
		pos = reSpell.indexIn(peekStr);
		if (pos > -1) {
			QString lang = reSpell.cap(1).trimmed();
			setSpellcheckLanguage(lang);
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
	QRegExp re("% *!TEX +root *= *([^\\x2029]+)\\x2029", Qt::CaseInsensitive);
	int pos = re.indexIn(peekStr);
	if (pos > -1) {
		rootName = re.cap(1).trimmed();
		QFileInfo rootFileInfo(fileInfo.canonicalPath() + "/" + rootName);
		if (rootFileInfo.exists())
			rootFilePath = rootFileInfo.canonicalFilePath();
		else
			rootFilePath = rootFileInfo.filePath();
	}
	else
		rootFilePath = fileInfo.canonicalFilePath();
}

void TeXDocument::addTag(const QTextCursor& cursor, int level, const QString& text)
{
	int index = 0;
	while (index < tags.size()) {
		if (tags[index].cursor.selectionStart() > cursor.selectionStart())
			break;
		++index;
	}
	tags.insert(index, Tag(cursor, level, text));
}

int TeXDocument::removeTags(int offset, int len)
{
	int removed = 0;
	for (int index = tags.count() - 1; index >= 0; --index) {
		if (tags[index].cursor.selectionStart() < offset)
			break;
		if (tags[index].cursor.selectionStart() < offset + len) {
			tags.removeAt(index);
			++removed;
		}
	}
	return removed;
}

void TeXDocument::goToTag(int index)
{
	if (index < tags.count()) {
		textEdit->setTextCursor(tags[index].cursor);
		textEdit->setFocus(Qt::OtherFocusReason);
	}
}

void TeXDocument::tagsChanged()
{
	if (deferTagListChanges)
		tagListChanged = true;
	else
		emit tagListUpdated();
}

void TeXDocument::removeAuxFiles()
{
	findRootFilePath();
	if (rootFilePath.isEmpty())
		return;

	QFileInfo fileInfo(rootFilePath);
	QString jobname = fileInfo.completeBaseName();
	QDir dir(fileInfo.dir());
	
	QStringList filterList = TWUtils::cleanupPatterns().split(QRegExp("\\s+"));
	if (filterList.count() == 0)
		return;
	for (int i = 0; i < filterList.count(); ++i)
		filterList[i].replace("$jobname", jobname);
	
	dir.setNameFilters(filterList);
	QStringList auxFileList = dir.entryList(QDir::Files | QDir::CaseSensitive, QDir::Name);
	if (auxFileList.count() > 0)
		ConfirmDelete::doConfirmDelete(dir, auxFileList);
	else
		(void)QMessageBox::information(this, tr("No files found"),
									   tr("No auxiliary files associated with this document at the moment."));
}

#ifdef Q_WS_MAC
#define OPEN_FILE_IN_NEW_WINDOW	Qt::MoveAction // unmodified drag appears as MoveAction on Mac OS X
#define INSERT_DOCUMENT_TEXT	Qt::CopyAction
#define CREATE_INCLUDE_COMMAND	Qt::LinkAction
#else
#define OPEN_FILE_IN_NEW_WINDOW	Qt::CopyAction // ...but as CopyAction on X11
#define INSERT_DOCUMENT_TEXT	Qt::MoveAction
#define CREATE_INCLUDE_COMMAND	Qt::LinkAction
#endif

void TeXDocument::dragEnterEvent(QDragEnterEvent *event)
{
	event->ignore();
	if (event->mimeData()->hasUrls()) {
		const QList<QUrl> urls = event->mimeData()->urls();
		foreach (const QUrl& url, urls) {
			if (url.scheme() == "file") {
				event->acceptProposedAction();
				break;
			}
		}
	}
}

void TeXDocument::dragMoveEvent(QDragMoveEvent *event)
{
	if (event->proposedAction() == INSERT_DOCUMENT_TEXT || event->proposedAction() == CREATE_INCLUDE_COMMAND) {
		if (dragSavedCursor.isNull())
			dragSavedCursor = textEdit->textCursor();
		QTextCursor curs = textEdit->cursorForPosition(textEdit->mapFromGlobal(mapToGlobal(event->pos())));
		textEdit->setTextCursor(curs);
	}
	else {
		if (!dragSavedCursor.isNull()) {
			textEdit->setTextCursor(dragSavedCursor);
			dragSavedCursor = QTextCursor();
		}
	}
	event->acceptProposedAction();
}

void TeXDocument::dragLeaveEvent(QDragLeaveEvent *event)
{
	if (!dragSavedCursor.isNull()) {
		textEdit->setTextCursor(dragSavedCursor);
		dragSavedCursor = QTextCursor();
	}
	event->accept();
}

void TeXDocument::dropEvent(QDropEvent *event)
{
	if (event->mimeData()->hasUrls()) {
		Qt::DropAction action = event->proposedAction();
		const QList<QUrl> urls = event->mimeData()->urls();
		bool editBlockStarted = false;
		QString text;
		QTextCursor curs = textEdit->cursorForPosition(textEdit->mapFromGlobal(mapToGlobal(event->pos())));
		foreach (const QUrl& url, urls) {
			if (url.scheme() == "file") {
				QString fileName = url.toLocalFile();
				switch (action) {
					case OPEN_FILE_IN_NEW_WINDOW:
						TWApp::instance()->openFile(fileName);
						break;

					case INSERT_DOCUMENT_TEXT:
						if (!TWUtils::isPDFfile(fileName) && !TWUtils::isImageFile(fileName) && !TWUtils::isPostscriptFile(fileName)) {
							QTextCodec *codecUsed;
							text = readFile(fileName, &codecUsed);
							if (!text.isNull()) {
								if (!editBlockStarted) {
									curs.beginEditBlock();
									editBlockStarted = true;
								}
								textEdit->setTextCursor(curs);
								curs.insertText(text);
							}
							break;
						}
						// for graphic files, fall through -- behave the same as the "link" action

					case CREATE_INCLUDE_COMMAND:
						if (!editBlockStarted) {
							curs.beginEditBlock();
							editBlockStarted = true;
						}
						textEdit->setTextCursor(curs);
						if (TWUtils::isPDFfile(fileName))
							text = TWUtils::includePdfCommand();
						else if (TWUtils::isImageFile(fileName))
							text = TWUtils::includeImageCommand();
						else if (TWUtils::isPostscriptFile(fileName))
							text = TWUtils::includePostscriptCommand();
						else
							text = TWUtils::includeTextCommand();
						curs.insertText(text.arg(fileName));
						break;
					default:
						// do nothing
						break;
				}
			}
		}
		if (editBlockStarted)
			curs.endEditBlock();
	}
	dragSavedCursor = QTextCursor();
	event->accept();
}

void TeXDocument::detachPdf()
{
	if (pdfDoc != NULL) {
		disconnect(pdfDoc, SIGNAL(destroyed()), this, SLOT(pdfClosed()));
		disconnect(this, SIGNAL(destroyed(QObject*)), pdfDoc, SLOT(texClosed(QObject*)));
		pdfDoc = NULL;
	}
}
