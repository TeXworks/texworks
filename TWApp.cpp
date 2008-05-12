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

#include "TWApp.h"
#include "TWUtils.h"
#include "TeXDocument.h"
#include "PDFDocument.h"
#include "PrefsDialog.h"
#include "TemplateDialog.h"

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

const int kDefaultMaxRecentFiles = 10;

TWApp *TWApp::theAppInstance = NULL;

TWApp::TWApp(int &argc, char **argv)
	: QApplication(argc, argv)
	, defaultCodec(NULL)
	, binaryPaths(NULL)
	, engineList(NULL)
	, defaultEngineIndex(0)
{
	init();
}

void TWApp::init()
{
	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName(TEXWORKS_NAME);

	setWindowIcon(QIcon(":/images/images/appicon.png"));

	QString locale = QLocale::system().name();
	QTranslator translator;
	translator.load(QString(TEXWORKS_NAME "_") + locale);
	installTranslator(&translator);

	QSettings settings;
	recentFilesLimit = settings.value("maxRecentFiles", kDefaultMaxRecentFiles).toInt();

	QString codecName = settings.value("defaultEncoding", "UTF-8").toString();
	defaultCodec = QTextCodec::codecForName(codecName.toAscii());
	if (defaultCodec == NULL)
		defaultCodec = QTextCodec::codecForName("UTF-8");

#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);

	extern void qt_mac_set_menubar_icons(bool);
	qt_mac_set_menubar_icons(false);

	menuBar = new QMenuBar;

	menuFile = menuBar->addMenu(tr("File"));

	QAction *actionNew = new QAction(tr("New"), this);
    actionNew->setShortcut(QKeySequence("Ctrl+N"));
	actionNew->setIcon(QIcon(":/images/tango/document-new.png"));
	menuFile->addAction(actionNew);
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));

	QAction *actionNew_from_Template = new QAction(tr("New from Template..."), this);
	actionNew_from_Template->setShortcut(QKeySequence("Ctrl+Shift+N"));
	menuFile->addAction(actionNew_from_Template);
	connect(actionNew_from_Template, SIGNAL(triggered()), this, SLOT(newFromTemplate()));

	QAction *actionPreferences = new QAction(tr("Preferences..."), this);
	actionPreferences->setIcon(QIcon(":/images/tango/preferences-system.png"));
	menuFile->addAction(actionPreferences);
	connect(actionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));

	QAction *actionOpen = new QAction(tr("Open..."), this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));
	actionOpen->setIcon(QIcon(":/images/tango/document-open.png"));
	menuFile->addAction(actionOpen);
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->addMenu(menuRecent);

	menuHelp = menuBar->addMenu(tr("Help"));

	QAction *aboutAction = new QAction(tr("About " TEXWORKS_NAME "..."), this);
	menuHelp->addAction(aboutAction);
	connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(about()));
#endif

	theAppInstance = this;
}

void TWApp::about()
{
   QMessageBox::about(NULL, tr("About " TEXWORKS_NAME),
			tr("<p>" TEXWORKS_NAME " is a simple environment for editing, "
			    "typesetting, and previewing TeX documents.</p>"
				"<p>&#xA9; 2007-2008 Jonathan Kew.</p>"
				"<p>Distributed under the GNU General Public License, version 2.</p>"
				));
}

void TWApp::newFile()
{
	TeXDocument *doc = new TeXDocument;
	doc->show();
}

void TWApp::newFromTemplate()
{
	QString templateName = TemplateDialog::doTemplateDialog();
	if (!templateName.isEmpty()) {
		TeXDocument *doc = new TeXDocument(templateName, true);
		if (doc != NULL) {
			doc->makeUntitled();
			doc->selectWindow();
		}
	}
}

void TWApp::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action)
		open(action->data().toString());
}

void TWApp::open()
{
	QString fileName = QFileDialog::getOpenFileName();
	open(fileName);
}

void TWApp::open(const QString &fileName)
{
	if (TWUtils::isPDFfile(fileName)) {
		PDFDocument *doc = PDFDocument::findDocument(fileName);
		if (doc == NULL)
			doc = new PDFDocument(fileName);
		if (doc != NULL)
			doc->selectWindow();
	}
	else
		TeXDocument::openDocument(fileName);
}

void TWApp::preferences()
{
	PrefsDialog::doPrefsDialog(activeWindow());
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

		QSettings settings;
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
}

void TWApp::tileWindows()
{
}

void TWApp::tileTwoWindows()
{
}

bool TWApp::event(QEvent *event)
{
	switch (event->type()) {
		case QEvent::FileOpen:
			open(static_cast<QFileOpenEvent *>(event)->file());        
			return true;
		default:
			return QApplication::event(event);
	}
}

void TWApp::setDefaultPaths()
{
	if (binaryPaths == NULL)
		binaryPaths = new QStringList;
	else
		binaryPaths->clear();
	*binaryPaths
#ifdef Q_WS_MAC
		<< "/usr/texbin"
		<< "/usr/local/bin"
		<< "/Volumes/Nenya/texlive/Master/bin/powerpc-darwin"
#endif
#ifdef Q_WS_X11
		<< "/usr/local/texlive/2008/bin/i386-linux"
		<< "/usr/local/texlive/2007/bin/i386-linux"
		<< "/usr/local/bin"
#endif
#ifdef Q_WS_WIN
		<< "c:/texlive/2008/bin"
		<< "c:/texlive/2007/bin"
		<< "c:/w32tex/bin"
#endif
		;
}

const QStringList TWApp::getBinaryPaths()
{
	if (binaryPaths == NULL) {
		binaryPaths = new QStringList;
		QSettings settings;
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
}

void TWApp::setDefaultEngineList()
{
	if (engineList == NULL)
		engineList = new QList<Engine>;
	else
		engineList->clear();
#ifdef Q_WS_WIN
#define EXE ".exe"
#else
#define EXE
#endif
	*engineList
		<< Engine("pdfTeX", "pdftex" EXE, QStringList(QStringList("-synctex=1") << "$fullname"), true)
		<< Engine("pdfLaTeX", "pdflatex" EXE, QStringList(QStringList("-synctex=1") << "$fullname"), true)
		<< Engine("XeTeX", "xetex" EXE, QStringList(QStringList("-synctex=1") << "$fullname"), true)
		<< Engine("XeLaTeX", "xelatex" EXE, QStringList(QStringList("-synctex=1") << "$fullname"), true)
		<< Engine("ConTeXt", "texexec" EXE, QStringList("$fullname"), true)
		<< Engine("BibTeX", "bibtex" EXE, QStringList("$basename"), false)
		<< Engine("MakeIndex", "makeindex" EXE, QStringList("$basename"), false);
	defaultEngineIndex = 1;
}

const QList<Engine> TWApp::getEngineList()
{
	if (engineList == NULL) {
		engineList = new QList<Engine>;
		QSettings settings;
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
			}
		}
		else
			setDefaultEngineList();
		settings.endArray();
		setDefaultEngine(settings.value("defaultEngine").toString());
	}
	return *engineList;
}

void TWApp::setEngineList(const QList<Engine>& engines)
{
	if (engineList == NULL)
		engineList = new QList<Engine>;
	*engineList = engines;
	QSettings settings;
	int i = settings.beginReadArray("engines");
	settings.endArray();
	settings.beginWriteArray("engines", engines.count());
	while (i > engines.count()) {
		settings.setArrayIndex(--i);
		settings.remove("");
	}
	i = 0;
	foreach (Engine eng, engines) {
		settings.setArrayIndex(i++);
		settings.setValue("name", eng.name());
		settings.setValue("program", eng.program());
		settings.setValue("arguments", eng.arguments());
		settings.setValue("showPdf", eng.showPdf());
	}
	settings.endArray();
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
			QSettings settings;
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
	foreach (Engine e, engines) {
		if (e.name().compare(name, Qt::CaseInsensitive) == 0)
			return e;
	}
	return Engine();
}

void TWApp::syncFromSource(const QString& sourceFile, int lineNo)
{
	emit syncPdf(sourceFile, lineNo);
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
		QSettings settings;
		settings.setValue("defaultEncoding", codec->name());
	}
}
