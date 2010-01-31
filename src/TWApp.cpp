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

#include "TWApp.h"
#include "TWUtils.h"
#include "TeXDocument.h"
#include "PDFDocument.h"
#include "PrefsDialog.h"
#include "TemplateDialog.h"

#include "TWVersion.h"
#include "SvnRev.h"

#ifndef Q_WS_WIN
#include "DefaultBinaryPaths.h"
#endif

#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>
#include <QKeySequence>
#include <QDesktopWidget>
#include <QTextCodec>
#include <QLocale>
#include <QTranslator>
#include <QUrl>
#include <QDesktopServices>

#if defined(HAVE_POPPLER_XPDF_HEADERS) && (defined(Q_WS_MAC) || defined(Q_WS_WIN))
#include "poppler-config.h"
#include "GlobalParams.h"
#endif

#define SETUP_FILE_NAME "texworks-setup.ini"

const int kDefaultMaxRecentFiles = 10;

TWApp *TWApp::theAppInstance = NULL;

TWApp::TWApp(int &argc, char **argv)
	: QApplication(argc, argv)
	, defaultCodec(NULL)
	, binaryPaths(NULL)
	, defaultBinPaths(NULL)
	, engineList(NULL)
	, defaultEngineIndex(0)
	, settingsFormat(QSettings::NativeFormat)
	, scriptManager(NULL)
#ifdef Q_WS_WIN
	, messageTargetWindow(NULL)
#endif
{
	init();
}

TWApp::~TWApp()
{
	if (scriptManager) {
		scriptManager->saveDisabledList();
		delete scriptManager;
	}
}

void TWApp::init()
{
	setWindowIcon(QIcon(":/images/images/TeXworks.png"));

	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName(TEXWORKS_NAME);
	
	// <Check for portable mode>
#ifdef Q_WS_MAC
	QDir appDir(applicationDirPath() + "/../../.."); // move up to dir containing the .app package
#else
	QDir appDir(applicationDirPath());
#endif
	QDir iniPath(appDir.absolutePath());
	QDir libPath(appDir.absolutePath());
	if (appDir.exists(SETUP_FILE_NAME)) {
		QSettings portable(appDir.filePath(SETUP_FILE_NAME), QSettings::IniFormat);
		if (portable.contains("inipath")) {
			if (iniPath.cd(portable.value("inipath").toString())) {
				setSettingsFormat(QSettings::IniFormat);
				QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, iniPath.absolutePath());
			}
		}
		if (portable.contains("libpath")) {
			if (libPath.cd(portable.value("libpath").toString())) {
				portableLibPath = libPath.absolutePath();
			}
		}
		if (portable.contains("defaultbinpaths")) {
			defaultBinPaths = new QStringList;
			*defaultBinPaths = portable.value("defaultbinpaths").toString().split(PATH_LIST_SEP, QString::SkipEmptyParts);
		}
	}
	const char *envPath;
	envPath = getenv("TW_INIPATH");
	if (envPath != NULL && iniPath.cd(QString(envPath))) {
		setSettingsFormat(QSettings::IniFormat);
		QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, iniPath.absolutePath());
	}
	envPath = getenv("TW_LIBPATH");
	if (envPath != NULL && libPath.cd(QString(envPath))) {
		portableLibPath = libPath.absolutePath();
	}
	// </Check for portable mode>

#if defined(HAVE_POPPLER_XPDF_HEADERS) && (defined(Q_WS_MAC) || defined(Q_WS_WIN))
	// for Mac and Windows, support "local" poppler-data directory
	// (requires patched poppler-qt4 lib to be effective,
	// otherwise the GlobalParams gets overwritten when a
	// document is opened)
#ifdef Q_WS_MAC
	QDir popplerDataDir(applicationDirPath() + "/../poppler-data");
#else
	QDir popplerDataDir(applicationDirPath() + "/poppler-data");
#endif
	if (popplerDataDir.exists()) {
		globalParams = new GlobalParams(popplerDataDir.canonicalPath().toUtf8().data());
	}
#endif

	// Required for TWUtils::getLibraryPath()
	theAppInstance = this;

	QSETTINGS_OBJECT(settings);
	
	QString locale = settings.value("locale", QLocale::system().name()).toString();
	applyTranslation(locale);

	recentFilesLimit = settings.value("maxRecentFiles", kDefaultMaxRecentFiles).toInt();

	QString codecName = settings.value("defaultEncoding", "UTF-8").toString();
	defaultCodec = QTextCodec::codecForName(codecName.toAscii());
	if (defaultCodec == NULL)
		defaultCodec = QTextCodec::codecForName("UTF-8");

	TWUtils::readConfig();

	scriptManager = new TWScriptManager;

#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);

	extern void qt_mac_set_menubar_icons(bool);
	qt_mac_set_menubar_icons(false);

	menuBar = new QMenuBar;

	menuFile = menuBar->addMenu(tr("File"));

	actionNew = new QAction(tr("New"), this);
	actionNew->setIcon(QIcon(":/images/tango/document-new.png"));
	menuFile->addAction(actionNew);
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));

	actionNew_from_Template = new QAction(tr("New from Template..."), this);
	menuFile->addAction(actionNew_from_Template);
	connect(actionNew_from_Template, SIGNAL(triggered()), this, SLOT(newFromTemplate()));

	actionPreferences = new QAction(tr("Preferences..."), this);
	actionPreferences->setIcon(QIcon(":/images/tango/preferences-system.png"));
	menuFile->addAction(actionPreferences);
	connect(actionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));

	actionOpen = new QAction(tr("Open..."), this);
	actionOpen->setIcon(QIcon(":/images/tango/document-open.png"));
	menuFile->addAction(actionOpen);
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->addMenu(menuRecent);

	menuHelp = menuBar->addMenu(tr("Help"));

	homePageAction = new QAction(tr("Go to TeXworks home page"), this);
	menuHelp->addAction(homePageAction);
	connect(homePageAction, SIGNAL(triggered()), this, SLOT(goToHomePage()));
	mailingListAction = new QAction(tr("Email to the mailing list"), this);
	menuHelp->addAction(mailingListAction);
	connect(mailingListAction, SIGNAL(triggered()), this, SLOT(writeToMailingList()));
	QAction* sep = new QAction(this);
	sep->setSeparator(true);
	menuHelp->addAction(sep);
	aboutAction = new QAction(tr("About " TEXWORKS_NAME "..."), this);
	aboutAction->setMenuRole(QAction::AboutRole);
	menuHelp->addAction(aboutAction);
	connect(aboutAction, SIGNAL(triggered()), this, SLOT(about()));
	
	TWUtils::insertHelpMenuItems(menuHelp);

	connect(this, SIGNAL(updatedTranslators()), this, SLOT(changeLanguage()));
	changeLanguage();
#endif
}

void TWApp::maybeQuit()
{
#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(true);
#endif
	closeAllWindows();
#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);
#endif
}

void TWApp::changeLanguage()
{
#ifdef Q_WS_MAC
	menuFile->setTitle(tr("File"));
	actionNew->setText(tr("New"));
	actionNew->setShortcut(QKeySequence(tr("Ctrl+N")));
	actionNew_from_Template->setText(tr("New from Template..."));
	actionNew_from_Template->setShortcut(QKeySequence(tr("Ctrl+Shift+N")));
	actionOpen->setText(tr("Open..."));
	actionOpen->setShortcut(QKeySequence(tr("Ctrl+O")));

	menuRecent->setTitle(tr("Open Recent"));

	menuHelp->setTitle(tr("Help"));
	aboutAction->setText(tr("About " TEXWORKS_NAME "..."));
	homePageAction->setText(tr("Go to TeXworks home page"));
	mailingListAction->setText(tr("Email to the mailing list"));
	TWUtils::insertHelpMenuItems(menuHelp);
#endif
}

void TWApp::about()
{
	QString aboutText = tr("<p>%1 is a simple environment for editing, typesetting, and previewing TeX documents.</p>").arg(TEXWORKS_NAME);
	aboutText += "<small>";
	aboutText += "<p>&#xA9; 2007-2010 Jonathan Kew &amp; Stefan L&#xF6;ffler";
	aboutText += tr("<br>Version %1 (r.%2)").arg(TEXWORKS_VERSION).arg(SVN_REVISION);
	aboutText += tr("<p>Distributed under the <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU General Public License</a>, version 2.");
	aboutText += tr("<p><a href=\"http://trolltech.com/products/\">Qt4</a> application framework by Qt Software, a division of Nokia Corporation.");
	aboutText += tr("<br><a href=\"http://poppler.freedesktop.org/\">Poppler</a> PDF rendering library by Kristian H&#xF8;gsberg, Albert Astals Cid and others.");
	aboutText += tr("<br><a href=\"http://hunspell.sourceforge.net/\">Hunspell</a> spell checker by L&#xE1;szl&#xF3; N&#xE9;meth.");
	aboutText += tr("<br>Concept and resources from <a href=\"http://www.uoregon.edu/~koch/texshop/\">TeXShop</a> by Richard Koch.");
	aboutText += tr("<br><a href=\"http://itexmac.sourceforge.net/SyncTeX.html\">SyncTeX</a> technology by J&#xE9;r&#xF4;me Laurens.");
	aboutText += tr("<br>Some icons used are from the <a href=\"http://tango.freedesktop.org/\">Tango Desktop Project</a>.");
	QString trText = tr("<p>%1 translation kindly contributed by %2.").arg(tr("[language name]")).arg(tr("[translator's name/email]"));
	if (!trText.contains("[language name]"))
		aboutText += trText;	// omit this if it hasn't been translated!
	aboutText += "</small>";
	QMessageBox::about(NULL, tr("About %1").arg(TEXWORKS_NAME), aboutText);
}

void TWApp::openUrl(const QUrl& url)
{
	if (!QDesktopServices::openUrl(url))
		QMessageBox::warning(NULL, TEXWORKS_NAME,
							 tr("Unable to access \"%1\"; perhaps your browser or mail application is not properly configured?")
							 .arg(url.toString()));
}

void TWApp::goToHomePage()
{
	openUrl(QUrl("http://texworks.org/"));
}

void TWApp::writeToMailingList()
{
	openUrl(QUrl("mailto:texworks@tug.org?subject=message from TeXworks user"));
}

void TWApp::launchAction()
{
	if (TeXDocument::documentList().size() > 0 || PDFDocument::documentList().size() > 0)
		return;

	QSETTINGS_OBJECT(settings);
	int launchOption = settings.value("launchOption", 1).toInt();
	switch (launchOption) {
		case 1: // Blank document
			newFile();
			break;
		case 2: // New from Template
			newFromTemplate();
			break;
		case 3: // Open File
			open();
			break;
	}
#ifndef Q_WS_MAC	// on Mac OS, it's OK to end up with no document (we still have the app menu bar)
					// but on W32 and X11 we need a window otherwise the user can't interact at all
	if (TeXDocument::documentList().size() == 0 && PDFDocument::documentList().size() == 0) {
		newFile();
		if (TeXDocument::documentList().size() == 0) {
			// something went wrong, give up!
			(void)QMessageBox::critical(NULL, tr("Unable to create window"),
					tr("Something is badly wrong; %1 was unable to create a document window. "
					   "The application will now quit.").arg(TEXWORKS_NAME),
					QMessageBox::Close, QMessageBox::Close);
			quit();
		}
	}
#endif
}

void TWApp::newFile()
{
	TeXDocument *doc = new TeXDocument;
	doc->show();
	doc->editor()->updateLineNumberAreaWidth(0);
}

void TWApp::newFromTemplate()
{
	QString templateName = TemplateDialog::doTemplateDialog();
	if (!templateName.isEmpty()) {
		TeXDocument *doc = new TeXDocument(templateName, true);
		if (doc != NULL) {
			doc->makeUntitled();
			doc->selectWindow();
			doc->editor()->updateLineNumberAreaWidth(0);
		}
	}
}

void TWApp::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		openFile(action->data().toString());
}

QStringList TWApp::getOpenFileNames()
{
	QSETTINGS_OBJECT(settings);
	QString lastOpenDir = settings.value("openDialogDir").toString();
	return QFileDialog::getOpenFileNames(NULL, QString(tr("Open File")), lastOpenDir, TWUtils::filterList()->join(";;"));
}

QString TWApp::getOpenFileName()
{
	QSETTINGS_OBJECT(settings);
	QString lastOpenDir = settings.value("openDialogDir").toString();
	return QFileDialog::getOpenFileName(NULL, QString(tr("Open File")), lastOpenDir, TWUtils::filterList()->join(";;"));
}

QString TWApp::getSaveFileName(const QString& defaultName)
{
#ifdef Q_WS_WIN
	QFileDialog::Options	options = QFileDialog::DontUseNativeDialog;
#else
	QFileDialog::Options	options = 0;
#endif
	QString selectedFilter;
	if (!TWUtils::filterList()->isEmpty())
		selectedFilter = TWUtils::filterList()->last();
	QString fileName = QFileDialog::getSaveFileName(NULL, tr("Save File"), defaultName,
													TWUtils::filterList()->join(";;"),
													&selectedFilter, options);
	if (!fileName.isEmpty()) {
		// add extension from the selected filter, if unique and not already present
		QRegExp re("\\(\\*(\\.[^ ]+)\\)");
		if (re.indexIn(selectedFilter) >= 0) {
			QString ext = re.cap(1);
			if (!fileName.endsWith(ext, Qt::CaseInsensitive) && !fileName.endsWith("."))
				fileName.append(ext);
		}
	}
	return fileName;
}

void TWApp::open()
{
	QSETTINGS_OBJECT(settings);
	QStringList files = getOpenFileNames();
	foreach (QString fileName, files) {
		if (!fileName.isEmpty()) {
			QFileInfo info(fileName);
			settings.setValue("openDialogDir", info.canonicalPath());
			openFile(fileName);
		}
	}
}

QObject* TWApp::openFile(const QString &fileName)
{
	if (TWUtils::isPDFfile(fileName)) {
		PDFDocument *doc = PDFDocument::findDocument(fileName);
		if (doc == NULL)
			doc = new PDFDocument(fileName);
		if (doc != NULL) {
			doc->selectWindow();
			return doc;
		}
		return NULL;
	}
	else
		return TeXDocument::openDocument(fileName);
}

void TWApp::preferences()
{
	PrefsDialog::doPrefsDialog(activeWindow());
}

void TWApp::emitHighlightLineOptionChanged()
{
	emit highlightLineOptionChanged();
}

int TWApp::maxRecentFiles() const
{
	return recentFilesLimit;
}

void TWApp::setMaxRecentFiles(int value)
{
	if (value < 1)
		value = 1;
	else if (value > 100)
		value = 100;

	if (value != recentFilesLimit) {
		recentFilesLimit = value;

		QSETTINGS_OBJECT(settings);
		settings.setValue("maxRecentFiles", value);

		updateRecentFileActions();
	}
}

void TWApp::updateRecentFileActions()
{
#ifdef Q_WS_MAC
	TWUtils::updateRecentFileActions(this, recentFileActions, menuRecent);	
#endif
	emit recentFileActionsChanged();
}

void TWApp::updateWindowMenus()
{
	emit windowListChanged();
}

void TWApp::stackWindows()
{
	arrangeWindows(TWUtils::stackWindowsInRect);
}

void TWApp::tileWindows()
{
	arrangeWindows(TWUtils::tileWindowsInRect);
}

void TWApp::arrangeWindows(TWUtils::WindowArrangementFunction func)
{
	QDesktopWidget *desktop = QApplication::desktop();
	for (int screenIndex = 0; screenIndex < desktop->numScreens(); ++screenIndex) {
		QWidgetList windows;
		foreach (TeXDocument* texDoc, TeXDocument::documentList())
			if (desktop->screenNumber(texDoc) == screenIndex)
				windows << texDoc;
		foreach (PDFDocument* pdfDoc, PDFDocument::documentList())
			if (desktop->screenNumber(pdfDoc) == screenIndex)
				windows << pdfDoc;
		if (windows.size() > 0)
			(*func)(windows, desktop->availableGeometry(screenIndex));
	}
}

bool TWApp::event(QEvent *event)
{
	switch (event->type()) {
		case QEvent::FileOpen:
			openFile(static_cast<QFileOpenEvent *>(event)->file());        
			return true;
		default:
			return QApplication::event(event);
	}
}

void TWApp::setDefaultPaths()
{
	QDir appDir(applicationDirPath());
	if (binaryPaths == NULL)
		binaryPaths = new QStringList;
	else
		binaryPaths->clear();
	if (defaultBinPaths)
		*binaryPaths = *defaultBinPaths;
#ifndef Q_WS_MAC
	// on OS X, this will be the path to {TW_APP_PACKAGE}/Contents/MacOS/
	// which doesn't make any sense as a search dir for TeX binaries
	if (!binaryPaths->contains(appDir.absolutePath()))
		binaryPaths->append(appDir.absolutePath());
#endif
	const char *envPath = getenv("PATH");
	if (envPath != NULL)
		foreach (const QString& s, QString(envPath).split(PATH_LIST_SEP, QString::SkipEmptyParts))
		if (!binaryPaths->contains(s))
			binaryPaths->append(s);
	if (!defaultBinPaths) {
#ifdef Q_WS_WIN
		*binaryPaths
			<< "c:/texlive/2009/bin"
			<< "c:/texlive/2008/bin"
			<< "c:/texlive/2007/bin"
			<< "c:/w32tex/bin"
			<< "c:/Program Files/MiKTeX 2.8/miktex/bin"
			<< "c:/Program Files (x86)/MiKTeX 2.8/miktex/bin"
			<< "c:/Program Files/MiKTeX 2.7/miktex/bin"
			<< "c:/Program Files (x86)/MiKTeX 2.7/miktex/bin"
		;
#else
		foreach (const QString& s, QString(DEFAULT_BIN_PATHS).split(PATH_LIST_SEP, QString::SkipEmptyParts))
			if (!binaryPaths->contains(s))
				binaryPaths->append(s);
#endif
	}
	for (int i = binaryPaths->count() - 1; i >= 0; --i) {
		QDir dir(binaryPaths->at(i));
		if (!dir.exists())
			binaryPaths->removeAt(i);
	}
	if (binaryPaths->count() == 0) {
		QMessageBox::warning(NULL, tr("No default binary directory found"),
			tr("None of the predefined directories for TeX-related programs could be found."
				"<p><small>To run any processes, you will need to set the binaries directory (or directories) "
				"for your TeX distribution using the Typesetting tab of the Preferences dialog."));
	}
}

const QStringList TWApp::getBinaryPaths()
{
	if (binaryPaths == NULL) {
		binaryPaths = new QStringList;
		QSETTINGS_OBJECT(settings);
		if (settings.contains("binaryPaths"))
			*binaryPaths = settings.value("binaryPaths").toStringList();
		else
			setDefaultPaths();
	}
	return *binaryPaths;
}

void TWApp::setBinaryPaths(const QStringList& paths)
{
	if (binaryPaths == NULL)
		binaryPaths = new QStringList;
	*binaryPaths = paths;
	QSETTINGS_OBJECT(settings);
	settings.setValue("binaryPaths", paths);
}

void TWApp::setDefaultEngineList()
{
	if (engineList == NULL)
		engineList = new QList<Engine>;
	else
		engineList->clear();
	*engineList
		<< Engine("pdfTeX", "pdftex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("pdfLaTeX", "pdflatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("XeTeX", "xetex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("XeLaTeX", "xelatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("ConTeXt", "texmfstart" EXE, QStringList("texexec") << "$fullname", true)
		<< Engine("XeConTeXt", "texmfstart" EXE, QStringList("texexec") << "--xtx" << "$fullname", true)
		<< Engine("BibTeX", "bibtex" EXE, QStringList("$basename"), false)
		<< Engine("MakeIndex", "makeindex" EXE, QStringList("$basename"), false);
	defaultEngineIndex = 1;
}

const QList<Engine> TWApp::getEngineList()
{
	if (engineList == NULL) {
		engineList = new QList<Engine>;
		bool foundList = false;
		// check for old engine list in Preferences
		QSETTINGS_OBJECT(settings);
		int count = settings.beginReadArray("engines");
		if (count > 0) {
			for (int i = 0; i < count; ++i) {
				settings.setArrayIndex(i);
				Engine eng;
				eng.setName(settings.value("name").toString());
				eng.setProgram(settings.value("program").toString());
				eng.setArguments(settings.value("arguments").toStringList());
				eng.setShowPdf(settings.value("showPdf").toBool());
				engineList->append(eng);
				settings.remove("");
			}
			foundList = true;
			saveEngineList();
		}
		settings.endArray();
		settings.remove("engines");

		if (!foundList) { // read engine list from config file
			QDir configDir(TWUtils::getLibraryPath("configuration"));
			QFile toolsFile(configDir.filePath("tools.ini"));
			if (toolsFile.exists()) {
				QSettings toolsSettings(toolsFile.fileName(), QSettings::IniFormat);
				QStringList toolNames = toolsSettings.childGroups();
				foreach (const QString& n, toolNames) {
					toolsSettings.beginGroup(n);
					Engine eng;
					eng.setName(toolsSettings.value("name").toString());
					eng.setProgram(toolsSettings.value("program").toString());
					eng.setArguments(toolsSettings.value("arguments").toStringList());
					eng.setShowPdf(toolsSettings.value("showPdf").toBool());
					engineList->append(eng);
					toolsSettings.endGroup();
				}
				foundList = true;
			}
		}

		if (!foundList)
			setDefaultEngineList();
		setDefaultEngine(settings.value("defaultEngine").toString());
	}
	return *engineList;
}

void TWApp::saveEngineList()
{
	QDir configDir(TWUtils::getLibraryPath("configuration"));
	QFile toolsFile(configDir.filePath("tools.ini"));
	QSettings toolsSettings(toolsFile.fileName(), QSettings::IniFormat);
	toolsSettings.clear();
	int n = 0;
	foreach (const Engine& e, *engineList) {
		toolsSettings.beginGroup(QString("%1").arg(++n, 3, 10, QChar('0')));
		toolsSettings.setValue("name", e.name());
		toolsSettings.setValue("program", e.program());
		toolsSettings.setValue("arguments", e.arguments());
		toolsSettings.setValue("showPdf", e.showPdf());
		toolsSettings.endGroup();
	}
}

void TWApp::setEngineList(const QList<Engine>& engines)
{
	if (engineList == NULL)
		engineList = new QList<Engine>;
	*engineList = engines;
	saveEngineList();
	QSETTINGS_OBJECT(settings);
	settings.setValue("defaultEngine", getDefaultEngine().name());
	emit engineListChanged();
}

const Engine TWApp::getDefaultEngine()
{
	const QList<Engine> engines = getEngineList();
	if (defaultEngineIndex < engines.count())
		return engines[defaultEngineIndex];
	defaultEngineIndex = 0;
	if (engines.empty())
		return Engine();
	else
		return engines[0];
}

void TWApp::setDefaultEngine(const QString& name)
{
	const QList<Engine> engines = getEngineList();
	int i;
	for (i = 0; i < engines.count(); ++i)
		if (engines[i].name() == name) {
			QSETTINGS_OBJECT(settings);
			settings.setValue("defaultEngine", name);
			break;
		}
	if (i == engines.count())
		i = 0;
	defaultEngineIndex = i;
}

const Engine TWApp::getNamedEngine(const QString& name)
{
	const QList<Engine> engines = getEngineList();
	foreach (const Engine& e, engines) {
		if (e.name().compare(name, Qt::CaseInsensitive) == 0)
			return e;
	}
	return Engine();
}

QTextCodec *TWApp::getDefaultCodec()
{
	return defaultCodec;
}

void TWApp::setDefaultCodec(QTextCodec *codec)
{
	if (codec == NULL)
		return;

	if (codec != defaultCodec) {
		defaultCodec = codec;
		QSETTINGS_OBJECT(settings);
		settings.setValue("defaultEncoding", codec->name());
	}
}

void TWApp::activatedWindow(QWidget* theWindow)
{
	emit hideFloatersExcept(theWindow);
}

void TWApp::applyTranslation(const QString& locale)
{
	foreach (QTranslator* t, translators) {
		removeTranslator(t);
		delete t;
	}
	translators.clear();

	if (!locale.isEmpty()) {
		QString basicTranslations = ":/resfiles/translations";
		QString extraTranslations = TWUtils::getLibraryPath("translations");
		
		QTranslator *qtTranslator = new QTranslator(this);
		if (qtTranslator->load("qt_" + locale, extraTranslations)) {
			installTranslator(qtTranslator);
			translators.append(qtTranslator);
		}
		else if (qtTranslator->load("qt_" + locale, basicTranslations)) {
			installTranslator(qtTranslator);
			translators.append(qtTranslator);
		}
		else
			delete qtTranslator;

		QTranslator *twTranslator = new QTranslator(this);
		if (twTranslator->load(TEXWORKS_NAME "_" + locale, extraTranslations)) {
			installTranslator(twTranslator);
			translators.append(twTranslator);
		}
		else if (twTranslator->load(TEXWORKS_NAME "_" + locale, basicTranslations)) {
			installTranslator(twTranslator);
			translators.append(twTranslator);
		}
		else
			delete twTranslator;
	}

	emit updatedTranslators();
}

void TWApp::addToRecentFiles(const QString& fileName)
{
	QFileInfo info(fileName);
	QString canonical = info.canonicalFilePath();
	if (canonical.isEmpty())
		return;

	QSETTINGS_OBJECT(settings);
	QStringList files = settings.value("recentFileList").toStringList();
	files.removeAll(fileName);
	files.prepend(fileName);
	while (files.size() > maxRecentFiles())
		files.removeLast();
	settings.setValue("recentFileList", files);
	updateRecentFileActions();
}

void TWApp::openHelpFile(const QString& helpDirName)
{
	QDir helpDir(helpDirName);
	if (helpDir.exists("index.html"))
		openUrl(QUrl::fromLocalFile(helpDir.absoluteFilePath("index.html")));
	else
		QMessageBox::warning(NULL, TEXWORKS_NAME, tr("Unable to find help file."));
}

void TWApp::updateScriptsList()
{
	scriptManager->clear();
	scriptManager->loadScripts();

	emit scriptListChanged();
}

void TWApp::showScriptsFolder()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(TWUtils::getLibraryPath("scripts")));
}

QVariant TWApp::launchFile(const QString& fileName, bool waitForResult)
{
	// first check if command execution is permitted
	QSETTINGS_OBJECT(settings);
	if (settings.value("allowSystemCommands", false).toBool())
		return waitForResult ? QDesktopServices::openUrl(QUrl::fromLocalFile(fileName)) : QVariant();
	else
		return waitForResult ? QVariant(tr("System command execution is disabled (see Preferences)")) : QVariant();
}

QVariant TWApp::system(const QString& cmdline, bool waitForResult)
{
	// first check if command execution is permitted
	QSETTINGS_OBJECT(settings);
	if (settings.value("allowSystemCommands", false).toBool()) {
		TWSystemCmd *process = new TWSystemCmd(this, waitForResult);
		connect(process, SIGNAL(readyReadStandardOutput()), process, SLOT(processOutput()));
		connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(processFinished(int, QProcess::ExitStatus)));
		connect(process, SIGNAL(error(QProcess::ProcessError)), process, SLOT(processError(QProcess::ProcessError)));
		if (waitForResult) {
			process->setProcessChannelMode(QProcess::MergedChannels);
			process->start(cmdline);
			if (!process->waitForStarted()) {
				process->deleteLater();
				return QVariant(tr("Failed to execute system command: %1").arg(cmdline));
			}
			if (!process->waitForFinished()) {
				process->deleteLater();
				return QVariant(tr("Error executing system command: %1").arg(cmdline));
			}
			return QVariant(process->getResult());
		}
		else {
			process->closeReadChannel(QProcess::StandardOutput);
			process->closeReadChannel(QProcess::StandardError);
			process->start(cmdline);
			return QVariant();
		}
	}
	else {
		if (waitForResult) {
			return QVariant(tr("System command execution is disabled (see Preferences)"));
		}
		// else result is null
		return QVariant();
	}
}

#ifdef Q_WS_WIN	// support for the Windows single-instance code
#include <windows.h>

LRESULT CALLBACK TW_HiddenWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) 
	{
		case WM_COPYDATA:
			{
				const COPYDATASTRUCT* pcds = (const COPYDATASTRUCT*)lParam;
				if (pcds->dwData == TW_OPEN_FILE_MSG) {
					if (TWApp::instance() != NULL) {
						QString fileName = QString::fromLocal8Bit((const char*)pcds->lpData, pcds->cbData);
						TWApp::instance()->openFile(fileName);
					}
				}
			}
			return 0;

		default:
			return DefWindowProc(hwnd, uMsg, wParam, lParam); 
	}
	return 0;
} 

void TWApp::createMessageTarget(QWidget* aWindow)
{
	if (messageTargetWindow != NULL)
		return;

	if (QCoreApplication::startingUp())
		return;

	if (!aWindow || !aWindow->isWindow())
		return;

	HINSTANCE hInstance = (HINSTANCE)GetWindowLongPtr(aWindow->winId(), GWLP_HINSTANCE);
	if (hInstance == NULL)
		return;

	WNDCLASSA myWindowClass;
	myWindowClass.style = 0;
	myWindowClass.lpfnWndProc = &TW_HiddenWindowProc;
	myWindowClass.cbClsExtra = 0;
	myWindowClass.cbWndExtra = 0;
	myWindowClass.hInstance = hInstance;
	myWindowClass.hIcon = NULL;
	myWindowClass.hCursor = NULL;
	myWindowClass.hbrBackground = NULL;
	myWindowClass.lpszMenuName = NULL;
	myWindowClass.lpszClassName = TW_HIDDEN_WINDOW_CLASS;

	ATOM atom = RegisterClassA(&myWindowClass);
	if (atom == 0)
		return;

	messageTargetWindow = CreateWindowA(TW_HIDDEN_WINDOW_CLASS, TEXWORKS_NAME, WS_OVERLAPPEDWINDOW,
					CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
					HWND_MESSAGE, NULL, hInstance, NULL);
}
#endif

#ifdef Q_WS_X11
void TWApp::bringToFront()
{
	foreach (QWidget* widget, topLevelWidgets()) {
		QMainWindow* window = qobject_cast<QMainWindow*>(widget);
		if (window != NULL) {
			window->raise();
			window->activateWindow();
		}
	}
}
#endif

