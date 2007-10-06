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
	f_maxRecentFiles = settings.contains("maxRecentFiles") ? settings.value("maxRecentFiles").toInt() : kDefaultMaxRecentFiles;

#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);

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
				"<p>Distributed under the GNU General Public License, version 2.</p>"
				"<p>&#xA9; 2007 Jonathan Kew.</p>"
				"<p>Built using the Qt toolkit from <a href=\"http://trolltech.com/\">Trolltech</a>.</p>"
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
