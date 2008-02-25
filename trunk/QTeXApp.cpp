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

QTeXApp::QTeXApp(int &argc, char **argv)
	: QApplication(argc, argv)
	, binaryPaths(NULL)
	, engineList(NULL)
	, defaultEngineIndex(0)
{
	init();
}

void QTeXApp::init()
{
	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName(TEXWORKS_NAME);

	setWindowIcon(QIcon(":/images/images/appicon.png"));

	QSettings settings;
	recentFilesLimit = settings.value("maxRecentFiles", kDefaultMaxRecentFiles).toInt();

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

	QAction *aboutAction = new QAction(tr("About " TEXWORKS_NAME "..."), this);
	menuHelp->addAction(aboutAction);
	connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(about()));
#endif
}

void QTeXApp::about()
{
   QMessageBox::about(NULL, tr("About " TEXWORKS_NAME),
			tr("<p>" TEXWORKS_NAME " is a simple environment for editing, "
			    "typesetting, and previewing TeX documents.</p>"
				"<p>&#xA9; 2007-2008 Jonathan Kew.</p>"
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
	return recentFilesLimit;
}

void QTeXApp::setMaxRecentFiles(int value)
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

void QTeXApp::setBinaryPaths(const QStringList& paths)
{
	if (binaryPaths == NULL)
		binaryPaths = new QStringList;
	*binaryPaths = paths;
}

void QTeXApp::setDefaultEngineList()
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

const QList<Engine> QTeXApp::getEngineList()
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
	}
	return *engineList;
}

void QTeXApp::setEngineList(const QList<Engine>& engines)
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
}

const Engine QTeXApp::getDefaultEngine()
{
	const QList<Engine> engines = getEngineList();
	if (defaultEngineIndex < engines.count())
		return engines[defaultEngineIndex];
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

void QTeXApp::syncFromSource(const QString& sourceFile, int lineNo)
{
	emit syncPdf(sourceFile, lineNo);
}
