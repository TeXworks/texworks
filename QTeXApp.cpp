#include "QTeXApp.h"
#include "QTeXUtils.h"
#include "TeXDocument.h"
#include "PDFDocument.h"
#include "PrefsDialog.h"

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

const int kDefaultMaxRecentFiles = 10;

QTeXApp::QTeXApp(int argc, char *argv[])
	: QApplication(argc, argv)
{
	init();
}

void QTeXApp::init()
{
	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName("TeXWorks");

	QSettings settings;
	f_maxRecentFiles = settings.value("maxRecentFiles", kDefaultMaxRecentFiles).toInt();

	f_binaryPaths = NULL;
	f_engineList = NULL;

#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);

	extern void qt_mac_set_menubar_icons(bool);
	qt_mac_set_menubar_icons(false);

	menuBar = new QMenuBar;

	menuFile = menuBar->addMenu(tr("File"));

	QAction *actionNew = new QAction(tr("New"), this);
    actionNew->setShortcut(QKeySequence("Ctrl+N"));
	actionNew->setIcon(QIcon(":/images/images/filenew.png"));
	menuFile->addAction(actionNew);
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));

	QAction *actionPreferences = new QAction(tr("Preferences..."), this);
	menuFile->addAction(actionPreferences);
	connect(actionPreferences, SIGNAL(triggered()), this, SLOT(preferences()));

	QAction *actionOpen = new QAction(tr("Open..."), this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));
	actionOpen->setIcon(QIcon(":/images/images/fileopen.png"));
	menuFile->addAction(actionOpen);
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));

	menuRecent = new QMenu(tr("Open Recent"));
	updateRecentFileActions();
	menuFile->addMenu(menuRecent);

	menuHelp = menuBar->addMenu(tr("Help"));

	QAction *aboutAction = new QAction(tr("About TeXWorks..."), this);
	menuHelp->addAction(aboutAction);
	connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(about()));
#endif
}

void QTeXApp::about()
{
   QMessageBox::about(activeWindow(), tr("About TeXWorks"),
			tr("<p>TeXWorks is a simple environment for editing, "
			    "typesetting, and previewing TeX documents.</p>"
				"<p>&#xA9; 2007 Jonathan Kew.</p>"
				"<p>Distributed under the GNU General Public License, version 2.</p>"
				));
}

void QTeXApp::newFile()
{
	TeXDocument *doc = new TeXDocument;
	doc->show();
}

void QTeXApp::openRecentFile()
{
	QAction *action = qobject_cast<QAction *>(sender());
	if (action) {
		TeXDocument *doc = qobject_cast<TeXDocument *>(activeWindow());
		if (doc)
			doc->open(action->data().toString());
		else
			open(action->data().toString());
	}
}

void QTeXApp::open()
{
	QString fileName = QFileDialog::getOpenFileName();
	open(fileName);
}

void QTeXApp::open(const QString &fileName)
{
	// TODO: support directly opening a PDF?
	TeXDocument::openDocument(fileName);
}

void QTeXApp::preferences()
{
	PrefsDialog::doPrefsDialog(activeWindow());
}

int QTeXApp::maxRecentFiles() const
{
	return f_maxRecentFiles;
}

void QTeXApp::setMaxRecentFiles(int value)
{
	if (value < 1)
		value = 1;
	else if (value > 100)
		value = 100;

	if (value != f_maxRecentFiles) {
		f_maxRecentFiles = value;

		QSettings settings;
		settings.setValue("maxRecentFiles", value);

		updateRecentFileActions();
	}
}

void QTeXApp::updateRecentFileActions()
{
#ifdef Q_WS_MAC
	QTeXUtils::updateRecentFileActions(this, recentFileActions, menuRecent);	
#endif
	emit recentFileActionsChanged();
}

void QTeXApp::updateWindowMenus()
{
	emit windowListChanged();
}

void QTeXApp::stackWindows()
{
}

void QTeXApp::tileWindows()
{
}

void QTeXApp::tileTwoWindows()
{
}

bool QTeXApp::event(QEvent *event)
{
	switch (event->type()) {
		case QEvent::FileOpen:
			open(static_cast<QFileOpenEvent *>(event)->file());        
			return true;
		default:
			return QApplication::event(event);
	}
}

void QTeXApp::setDefaultPaths()
{
	if (f_binaryPaths == NULL)
		f_binaryPaths = new QStringList;
	else
		f_binaryPaths->clear();
	*f_binaryPaths
#ifdef Q_WS_MAC
		<< "/usr/texbin"
		<< "/usr/local/bin"
		<< "/Volumes/Nenya/texlive/Master/bin/powerpc-darwin"
#endif
#ifdef Q_WS_X11
		<< "/usr/local/texlive/2007/bin/i386-linux"
		<< "/usr/local/bin"
#endif
#ifdef Q_WS_WIN
		<< "c:/texlive/2007/bin"
		<< "c:/w32tex/bin"
#endif
		;
}

const QStringList QTeXApp::getBinaryPaths()
{
	if (f_binaryPaths == NULL) {
		f_binaryPaths = new QStringList;
		QSettings settings;
		if (settings.contains("binaryPaths"))
			*f_binaryPaths = settings.value("binaryPaths").toStringList();
		else
			setDefaultPaths();
	}
	return *f_binaryPaths;
}

void QTeXApp::setBinaryPaths(const QStringList& paths)
{
	if (f_binaryPaths == NULL)
		f_binaryPaths = new QStringList;
	*f_binaryPaths = paths;
}

void QTeXApp::setDefaultEngineList()
{
	if (f_engineList == NULL)
		f_engineList = new QList<Engine>;
	else
		f_engineList->clear();
#ifdef Q_WS_WIN
#define EXE ".exe"
#else
#define EXE
#endif
	*f_engineList
		<< Engine("pdfTeX", "pdftex" EXE, QStringList("$fullname"), true)
		<< Engine("pdfLaTeX", "pdflatex" EXE, QStringList("$fullname"), true)
		<< Engine("XeTeX", "xetex" EXE, QStringList("$fullname"), true)
		<< Engine("XeLaTeX", "xelatex" EXE, QStringList("$fullname"), true)
		<< Engine("ConTeXt", "texexec" EXE, QStringList("$fullname"), true)
		<< Engine("BibTeX", "bibtex" EXE, QStringList("$basename"), false)
		<< Engine("MakeIndex", "makeindex" EXE, QStringList("$basename"), false);
	f_defaultEngineIndex = 1;
}

const QList<Engine> QTeXApp::getEngineList()
{
	if (f_engineList == NULL) {
		f_engineList = new QList<Engine>;
		QSettings settings;
		if (settings.childGroups().contains("engines")) {
			settings.beginGroup("engines");
			QStringList childKeys = settings.childKeys();
			foreach (QString engineName, childKeys) {
				Engine eng;
				eng.setName(engineName);
				settings.beginGroup(engineName);
				eng.setProgram(settings.value("program").toString());
				eng.setArguments(settings.value("arguments").toStringList());
				eng.setShowPdf(settings.value("showPdf").toBool());
				settings.endGroup();
				f_engineList->append(eng);
			}
			settings.endGroup();
		}
		else
			setDefaultEngineList();
	}
	return *f_engineList;
}

void QTeXApp::setEngineList(const QList<Engine>& engineList)
{
	if (f_engineList == NULL)
		f_engineList = new QList<Engine>;
	*f_engineList = engineList;
}

const Engine QTeXApp::getDefaultEngine()
{
	const QList<Engine> engines = getEngineList();
	if (f_defaultEngineIndex < engines.count())
		return engines[f_defaultEngineIndex];
	else if (engines.empty())
		return Engine();
	else
		return engines[0];
}

const Engine QTeXApp::getNamedEngine(const QString& name)
{
	const QList<Engine> engines = getEngineList();
	foreach (Engine e, engines) {
		if (e.name().compare(name, Qt::CaseInsensitive) == 0)
			return e;
	}
	return Engine();
}
