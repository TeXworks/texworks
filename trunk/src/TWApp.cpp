/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2011  Jonathan Kew, Stefan Löffler

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
	see <http://www.tug.org/texworks/>.
*/

#include "TWApp.h"
#include "TWUtils.h"
#include "TeXDocument.h"
#include "PDFDocument.h"
#include "PrefsDialog.h"
#include "TemplateDialog.h"
#include "TWSystemCmd.h"

#include "TWVersion.h"
#include "SvnRev.h"
#include "ResourcesDialog.h"

#ifdef Q_WS_WIN
#include "DefaultBinaryPathsWin.h"
#else
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

#ifdef Q_WS_MAC
#include <CoreServices/CoreServices.h>
#endif

#ifdef Q_WS_WIN
#include <windows.h>
#ifndef VER_SUITE_WH_SERVER /* not defined in my mingw system */
#define VER_SUITE_WH_SERVER 0x00008000
#endif
#endif

#define SETUP_FILE_NAME "texworks-setup.ini"

const int kDefaultMaxRecentFiles = 20;

TWApp *TWApp::theAppInstance = NULL;

TWApp::TWApp(int &argc, char **argv)
	: ConfigurableApp(argc, argv)
	, defaultCodec(NULL)
	, binaryPaths(NULL)
	, defaultBinPaths(NULL)
	, engineList(NULL)
	, defaultEngineIndex(0)
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
	QIcon appIcon;
#ifdef Q_WS_X11
	// The Compiz window manager doesn't seem to support icons larger than
	// 128x128, so we add a suitable one first
	appIcon.addFile(":/images/images/TeXworks-128.png");
#endif
	appIcon.addFile(":/images/images/TeXworks.png");
	setWindowIcon(appIcon);
	
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
	else {
		globalParams = new GlobalParams();
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
	actionClear_Recent_Files = menuRecent->addAction(tr("Clear Recent Files"));
	actionClear_Recent_Files->setEnabled(false);
	connect(actionClear_Recent_Files, SIGNAL(triggered()), this, SLOT(clearRecentFiles()));
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
	aboutText += "<p>&#xA9; 2007-2011  Jonathan Kew, Stefan L&#xF6;ffler";
	aboutText += tr("<br>Version %1 r.%2 (%3)").arg(TEXWORKS_VERSION).arg(SVN_REVISION).arg(TW_BUILD_ID_STR);
	aboutText += tr("<p>Distributed under the <a href=\"http://www.gnu.org/licenses/gpl-2.0.html\">GNU General Public License</a>, version 2 or (at your option) any later version.");
	aboutText += tr("<p><a href=\"http://qt.nokia.com/\">Qt application framework</a> v%1 by Qt Software, a division of Nokia Corporation.").arg(qVersion());
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
	openUrl(QUrl("http://www.tug.org/texworks/"));
}

#ifdef Q_WS_WIN
/* based on MSDN sample code from http://msdn.microsoft.com/en-us/library/ms724429(VS.85).aspx */
typedef void (WINAPI *PGNSI)(LPSYSTEM_INFO);

QString TWApp::GetWindowsVersionString()
{
	OSVERSIONINFOEXA osvi;
	SYSTEM_INFO si;
	PGNSI pGNSI;
	BOOL bOsVersionInfoEx;
	QString result("(unknown version)");
	
	memset(&si, 0, sizeof(SYSTEM_INFO));
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXA));
	
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	if ( !(bOsVersionInfoEx = GetVersionExA ((OSVERSIONINFOA *) &osvi)) )
		return result;
	
	// Call GetNativeSystemInfo if supported or GetSystemInfo otherwise.
	pGNSI = (PGNSI) GetProcAddress(GetModuleHandle(TEXT("kernel32.dll")), "GetNativeSystemInfo");
	if (NULL != pGNSI)
		pGNSI(&si);
	else
		GetSystemInfo(&si);
	
	if ( VER_PLATFORM_WIN32_NT == osvi.dwPlatformId && osvi.dwMajorVersion > 4 ) {
		if ( osvi.dwMajorVersion == 6 ) {
			if ( osvi.dwMinorVersion == 0 ) {
				if ( osvi.wProductType == VER_NT_WORKSTATION )
					result = "Vista";
				else
					result = "Server 2008";
			}
			else if ( osvi.dwMinorVersion == 1 ) {
				if( osvi.wProductType == VER_NT_WORKSTATION )
					result = "7";
				else
					result = "Server 2008 R2";
			}
		}
		else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 2 ) {
			if ( GetSystemMetrics(SM_SERVERR2) )
				result = "Server 2003 R2";
			else if ( osvi.wSuiteMask & VER_SUITE_STORAGE_SERVER )
				result = "Storage Server 2003";
			else if ( osvi.wSuiteMask & VER_SUITE_WH_SERVER )
				result = "Home Server";
			else if ( osvi.wProductType == VER_NT_WORKSTATION &&
					si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64)
				result = "XP Professional x64 Edition";
			else
				result = "Server 2003";
		}
		else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 1 ) {
			result = "XP ";
			if ( osvi.wSuiteMask & VER_SUITE_PERSONAL )
				result += "Home Edition";
			else
				result += "Professional";
		}
		else if ( osvi.dwMajorVersion == 5 && osvi.dwMinorVersion == 0 ) {
			result = "2000 ";
			
			if ( osvi.wProductType == VER_NT_WORKSTATION ) {
				result += "Professional";
			}
			else {
				if ( osvi.wSuiteMask & VER_SUITE_DATACENTER )
					result += "Datacenter Server";
				else if ( osvi.wSuiteMask & VER_SUITE_ENTERPRISE )
					result += "Advanced Server";
				else
					result += "Server";
			}
		}
		
		if ( strlen(osvi.szCSDVersion) > 0 ) {
			result += " ";
			result += osvi.szCSDVersion;
		}
		
		if ( osvi.dwMajorVersion >= 6 ) {
			if ( si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_AMD64 )
				result += ", 64-bit";
			else if (si.wProcessorArchitecture==PROCESSOR_ARCHITECTURE_INTEL )
				result += ", 32-bit";
		}
	}
	
	return result;
}

unsigned int TWApp::GetWindowsVersion()
{
	OSVERSIONINFOEXA osvi;
	
	memset(&osvi, 0, sizeof(OSVERSIONINFOEXA));
	
	osvi.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEXA);
	if ( !GetVersionExA ((OSVERSIONINFOA *) &osvi) )
		return 0;
	
	return (osvi.dwMajorVersion << 24) | (osvi.dwMinorVersion << 16) | (osvi.wServicePackMajor << 8) | (osvi.wServicePackMinor << 0);
}
#endif

const QStringList TWApp::getBinaryPaths(QStringList& systemEnvironment)
{
#ifdef Q_WS_WIN
#define PATH_CASE_SENSITIVE	Qt::CaseInsensitive
#else
#define PATH_CASE_SENSITIVE	Qt::CaseSensitive
#endif
	QStringList binPaths = getPrefsBinaryPaths();
	QMutableStringListIterator envIter(systemEnvironment);
	while (envIter.hasNext()) {
		QString& envVar = envIter.next();
		if (envVar.startsWith("PATH=", PATH_CASE_SENSITIVE)) {
			foreach (const QString& s, envVar.mid(5).split(QChar(PATH_LIST_SEP), QString::SkipEmptyParts)) {
				if (!binPaths.contains(s)) {
					binPaths.append(s);
				}
			}
			envVar = envVar.left(5) + binPaths.join(QChar(PATH_LIST_SEP));
			break;
		}
	}
	return binPaths;
}

QString TWApp::findProgram(const QString& program, const QStringList& binPaths)
{
	QStringListIterator pathIter(binPaths);
	bool found = false;
	QFileInfo fileInfo;
#ifdef Q_WS_WIN
	QStringList executableTypes = QStringList() << "exe" << "com" << "cmd" << "bat";
#endif
	while (pathIter.hasNext() && !found) {
		QString path = pathIter.next();
		fileInfo = QFileInfo(path, program);
		found = fileInfo.exists() && fileInfo.isExecutable();
#ifdef Q_WS_WIN
		// try adding common executable extensions, if one was not already present
		if (!found && !executableTypes.contains(fileInfo.suffix())) {
			QStringListIterator extensions(executableTypes);
			while (extensions.hasNext() && !found) {
				fileInfo = QFileInfo(path, program + "." + extensions.next());
				found = fileInfo.exists() && fileInfo.isExecutable();
			}
		}
#endif
	}
	return found ? fileInfo.absoluteFilePath() : QString();
}

void TWApp::writeToMailingList()
{
	// The strings here are deliberately NOT localizable!
	QString address("texworks@tug.org");
	QString body("Instructions:\n-) Please write your message in English (it's in your own best interest; otherwise, many people will not be able to understand it and therefore will not answer).\n\n-) Please type something meaningful in the subject line.\n\n-) If you are having a problem, please describe it step-by-step in detail.\n\n-) After reading, please delete these instructions (up to the \"configuration info\" below which we may need to find the source of problems).\n\n\n\n----- configuration info -----\n");

	body += "TeXworks version : " TEXWORKS_VERSION "r" SVN_REVISION_STR " (" TW_BUILD_ID_STR ")\n";
#ifdef Q_WS_MAC
	body += "Install location : " + QDir(applicationDirPath() + "/../..").absolutePath() + "\n";
#else
	body += "Install location : " + applicationFilePath() + "\n";
#endif
	body += "Library path     : " + TWUtils::getLibraryPath(QString()) + "\n";

	QStringList sysEnv(QProcess::systemEnvironment());
	const QStringList binPaths = getBinaryPaths(sysEnv);
	QString pdftex = findProgram("pdftex", binPaths);
	if (pdftex.isEmpty())
		pdftex = "not found";
	else {
		QFileInfo info(pdftex);
		pdftex = info.canonicalFilePath();
	}
	
	body += "pdfTeX location  : " + pdftex + "\n";
	
	body += "Operating system : ";
#ifdef Q_WS_WIN
	body += "Windows " + GetWindowsVersionString() + "\n";
#else
#ifdef Q_WS_MAC
#define UNAME_CMDLINE "uname -v"
#else
#define UNAME_CMDLINE "uname -a"
#endif
	QString unameResult("unknown");
	TWSystemCmd unameCmd(this, true);
	unameCmd.setProcessChannelMode(QProcess::MergedChannels);
	unameCmd.start(UNAME_CMDLINE);			
	if (unameCmd.waitForStarted(1000) && unameCmd.waitForFinished(1000))
		unameResult = unameCmd.getResult().trimmed();
#ifdef Q_WS_MAC
	SInt32 major = 0, minor = 0, bugfix = 0;
	Gestalt(gestaltSystemVersionMajor, &major);
	Gestalt(gestaltSystemVersionMinor, &minor);
	Gestalt(gestaltSystemVersionBugFix, &bugfix);
	body += QString("Mac OS X %1.%2.%3").arg(major).arg(minor).arg(bugfix);
	body += " (" + unameResult + ")\n";
#else
	body += unameResult + "\n";
#endif
#endif

	body += "Qt4 version      : " QT_VERSION_STR " (build) / ";
	body += qVersion();
	body += " (runtime)\n";
	body += "------------------------------\n";

	openUrl(QUrl(QString("mailto:%1?body=%3").arg(address).arg(body)));
}

void TWApp::launchAction()
{
	scriptManager->runHooks("TeXworksLaunched");

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

QObject * TWApp::newFile() const
{
	TeXDocument *doc = new TeXDocument;
	doc->show();
	doc->editor()->updateLineNumberAreaWidth(0);
	doc->runHooks("NewFile");
	return doc;
}

QObject * TWApp::newFromTemplate() const
{
	QString templateName = TemplateDialog::doTemplateDialog();
	if (!templateName.isEmpty()) {
		TeXDocument *doc = new TeXDocument(templateName, true);
		if (doc != NULL) {
			doc->makeUntitled();
			doc->selectWindow();
			doc->editor()->updateLineNumberAreaWidth(0);
			doc->runHooks("NewFromTemplate");
			return doc;
		}
	}
	return NULL;
}

void TWApp::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		openFile(action->data().toString());
}

QStringList TWApp::getOpenFileNames(QString selectedFilter)
{
	QFileDialog::Options	options = 0;
#ifdef Q_WS_WIN
	if(TWApp::GetWindowsVersion() < 0x06000000) options |= QFileDialog::DontUseNativeDialog;
#endif
	QSETTINGS_OBJECT(settings);
	QString lastOpenDir = settings.value("openDialogDir").toString();
	QStringList filters = *TWUtils::filterList();
	if (!selectedFilter.isNull() && !filters.contains(selectedFilter))
		filters.prepend(selectedFilter);
	return QFileDialog::getOpenFileNames(NULL, QString(tr("Open File")), lastOpenDir,
										 filters.join(";;"), &selectedFilter, options);
}

QString TWApp::getOpenFileName(QString selectedFilter)
{
	QFileDialog::Options	options = 0;
#ifdef Q_WS_WIN
	if(TWApp::GetWindowsVersion() < 0x06000000) options |= QFileDialog::DontUseNativeDialog;
#endif
	QSETTINGS_OBJECT(settings);
	QString lastOpenDir = settings.value("openDialogDir").toString();
	QStringList filters = *TWUtils::filterList();
	if (!selectedFilter.isNull() && !filters.contains(selectedFilter))
		filters.prepend(selectedFilter);
	return QFileDialog::getOpenFileName(NULL, QString(tr("Open File")), lastOpenDir,
										filters.join(";;"), &selectedFilter, options);
}

QString TWApp::getSaveFileName(const QString& defaultName)
{
	QFileDialog::Options	options = 0;
#ifdef Q_WS_WIN
	if(TWApp::GetWindowsVersion() < 0x06000000) options |= QFileDialog::DontUseNativeDialog;
#endif
	QString selectedFilter;
	if (!TWUtils::filterList()->isEmpty())
		selectedFilter = TWUtils::chooseDefaultFilter(defaultName, *(TWUtils::filterList()));
		
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

QObject* TWApp::openFile(const QString &fileName, int pos /* = 0 */)
{
	if (TWUtils::isPDFfile(fileName)) {
		PDFDocument *doc = PDFDocument::findDocument(fileName);
		if (doc == NULL)
			doc = new PDFDocument(fileName);
		if (doc != NULL) {
			if (pos > 0)
				doc->widget()->goToPage(pos - 1);
			doc->selectWindow();
			return doc;
		}
		return NULL;
	}
	else
		return TeXDocument::openDocument(fileName, true, true, pos, 0, 0);
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
	TWUtils::updateRecentFileActions(this, recentFileActions, menuRecent, actionClear_Recent_Files);
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
		foreach (const QString& s, QString(DEFAULT_BIN_PATHS).split(PATH_LIST_SEP, QString::SkipEmptyParts)) {
			if (!binaryPaths->contains(s))
				binaryPaths->append(s);
		}
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

const QStringList TWApp::getPrefsBinaryPaths()
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
//		<< Engine("LaTeXmk", "latexmk" EXE, QStringList("-e") << 
//				  "$pdflatex=q/pdflatex -synctex=1 %O %S/" << "-pdf" << "$fullname", true)
		<< Engine("pdfTeX", "pdftex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("pdfLaTeX", "pdflatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("LuaTeX", "luatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("LuaLaTeX", "lualatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("XeTeX", "xetex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("XeLaTeX", "xelatex" EXE, QStringList("$synctexoption") << "$fullname", true)
		<< Engine("ConTeXt (LuaTeX)", "context" EXE, QStringList("--synctex") << "$fullname", true)
		<< Engine("ConTeXt (pdfTeX)", "texexec" EXE, QStringList("--synctex") << "$fullname", true)
		<< Engine("ConTeXt (XeTeX)", "texexec" EXE, QStringList("--synctex") << "--xtx" << "$fullname", true)
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
		setDefaultEngine(settings.value("defaultEngine", DEFAULT_ENGINE_NAME).toString());
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
	for (i = 0; i < engines.count(); ++i) {
		if (engines[i].name() == name) {
			QSETTINGS_OBJECT(settings);
			settings.setValue("defaultEngine", name);
			break;
		}
	}
	// If the engine was not found (e.g., if it has been deleted)
	// try the DEFAULT_ENGINE_NAME instead (should not happen, unless the config
	// was edited manually (or by an updater, copy/paste'ing, etc.)
	if (i == engines.count() && name != DEFAULT_ENGINE_NAME) {
		for (i = 0; i < engines.count(); ++i) {
			if (engines[i].name() == DEFAULT_ENGINE_NAME) 
				break;
		}
	}
	// if neither the passed engine name nor DEFAULT_ENGINE_NAME was found,
	// fall back to selecting the first engine
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

void TWApp::addToRecentFiles(const QMap<QString,QVariant>& fileProperties)
{
	QSETTINGS_OBJECT(settings);

	QString fileName = fileProperties.value("path").toString();
	if (fileName.isEmpty())
		return;
	
	QList<QVariant> fileList = settings.value("recentFiles").toList();
	QList<QVariant>::iterator i = fileList.begin();
	while (i != fileList.end()) {
		QMap<QString,QVariant> h = i->toMap();
		if (h.value("path").toString() == fileName)
			i = fileList.erase(i);
		else
			++i;
	}

	fileList.prepend(fileProperties);

	while (fileList.size() > maxRecentFiles())
		fileList.removeLast();

	settings.setValue("recentFiles", QVariant::fromValue(fileList));

	updateRecentFileActions();
}

void TWApp::clearRecentFiles()
{
	QSETTINGS_OBJECT(settings);
	QList<QVariant> fileList;
	settings.setValue("recentFiles", QVariant::fromValue(fileList));
	updateRecentFileActions();
}

QMap<QString,QVariant> TWApp::getFileProperties(const QString& path)
{
	QSETTINGS_OBJECT(settings);
	QList<QVariant> fileList = settings.value("recentFiles").toList();
	QList<QVariant>::iterator i = fileList.begin();
	while (i != fileList.end()) {
		QMap<QString,QVariant> h = i->toMap();
		if (h.value("path").toString() == path)
			return h;
		++i;
	}
	return QMap<QString,QVariant>();
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
	scriptManager->reloadScripts();

	emit scriptListChanged();
}

void TWApp::showScriptsFolder()
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(TWUtils::getLibraryPath("scripts")));
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
						QStringList data = QString::fromUtf8((const char*)pcds->lpData, pcds->cbData).split('\n');
						if (data.size() == 1)
							TWApp::instance()->openFile(data[0]);
						else
							TWApp::instance()->openFile(data[0], data[1].toInt());
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

QList<QVariant> TWApp::getOpenWindows() const
{
	QList<QVariant> result;
	
	foreach (QWidget *widget, QApplication::topLevelWidgets()) {
		if (qobject_cast<TWScriptable*>(widget))
			result << QVariant::fromValue(qobject_cast<QObject*>(widget));
	}
	return result;
}

void TWApp::setGlobal(const QString& key, const QVariant& val)
{
	QVariant v = val;
	
	if (key.isEmpty())
		return;
	
	// For objects on the heap make sure we are notified when their lifetimes
	// end so that we can remove them from our hash accordingly
	switch ((QMetaType::Type)val.type()) {
		case QMetaType::QObjectStar:
			connect(v.value<QObject*>(), SIGNAL(destroyed(QObject*)), this, SLOT(globalDestroyed(QObject*)));
			break;
		case QMetaType::QWidgetStar:
			connect((QWidget*)v.data(), SIGNAL(destroyed(QObject*)), this, SLOT(globalDestroyed(QObject*)));
			break;
		default: break;
	}
	m_globals[key] = v;
}

void TWApp::globalDestroyed(QObject * obj)
{
	QHash<QString, QVariant>::iterator i = m_globals.begin();
	
	while (i != m_globals.end()) {
		switch ((QMetaType::Type)i.value().type()) {
			case QMetaType::QObjectStar:
				if (i.value().value<QObject*>() == obj)
					i = m_globals.erase(i);
				else
					++i;
				break;
			case QMetaType::QWidgetStar:
				if (i.value().value<QWidget*>() == obj)
					i = m_globals.erase(i);
				else
					++i;
				break;
			default:
				++i;
				break;
		}
	}
}

/*Q_INVOKABLE static*/
int TWApp::getVersion()
{
	return (VER_MAJOR << 16) | (VER_MINOR << 8) | VER_BUGFIX;
}

//Q_INVOKABLE
QMap<QString, QVariant> TWApp::openFileFromScript(const QString& fileName, QObject * scriptApiObj, const int pos /* = -1 */, const bool askUser /* = false */)
{
	QSETTINGS_OBJECT(settings);
	QMap<QString, QVariant> retVal;
	QObject * doc = NULL;
	TWScript * script;
	QFileInfo fi(fileName);
	TWScriptAPI * scriptApi = qobject_cast<TWScriptAPI*>(scriptApiObj);

	retVal["status"] = TWScriptAPI::SystemAccess_PermissionDenied;

	// for absolute paths and full reading permissions, we don't have to care
	// about peculiarities of the script; in that case, this even succeeds
	// if no valid scriptApi is passed; otherwise, we need to investigate further
	if (fi.isRelative() || !settings.value("allowScriptFileReading", false).toBool()) {
		if (!scriptApi)
			return retVal;
		script = qobject_cast<TWScript*>(scriptApi->GetScript());
		if (!script)
			return retVal; // this should never happen
	
		// relative paths are taken to be relative to the folder containing the
		// executing script's file
		QDir scriptDir(QFileInfo(script->getFilename()).dir());
		QString path = scriptDir.absoluteFilePath(fileName);
	
		if (!script->mayReadFile(path, scriptApi->GetTarget())) {
			// Possibly ask user to override the permissions
			if (!askUser)
				return retVal;
			if (QMessageBox::warning(qobject_cast<QWidget*>(scriptApi->GetTarget()), 
				tr("Permission request"),
				tr("The script \"%1\" is trying to open the file \"%2\" without sufficient permissions. Do you want to open the file?")\
					.arg(script->getTitle()).arg(path),
				QMessageBox::Yes | QMessageBox::No, QMessageBox::No
			) != QMessageBox::Yes)
				return retVal;
		}
	}
	doc = openFile(fileName, pos);
	retVal["result"] = QVariant::fromValue(doc);
	retVal["status"] = (doc != NULL ? TWScriptAPI::SystemAccess_OK : TWScriptAPI::SystemAccess_Failed);
	return retVal;
}

void TWApp::doResourcesDialog() const
{
	ResourcesDialog::doResourcesDialog(NULL);
}

