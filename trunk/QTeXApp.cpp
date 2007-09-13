#include "QTeXApp.h"

#include "TeXDocument.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QStringList>
#include <QKeySequence>

QTeXApp::QTeXApp(int argc, char *argv[])
	: QApplication(argc, argv)
{
	init();
}

void QTeXApp::init()
{
	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName("QTeX");

	for (int i = 0; i < kMaxRecentFiles; ++i) {
		recentFileActs[i] = new QAction(this);
		recentFileActs[i]->setVisible(false);
		connect(recentFileActs[i], SIGNAL(triggered()), this, SLOT(openRecentFile()));
	}

	menuRecent = new QMenu(tr("Open Recent"));
	for (int i = 0; i < kMaxRecentFiles; ++i)
		menuRecent->addAction(recentFileActs[i]);
	updateRecentFileActions();

#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);

	menuBar = new QMenuBar;

	menuFile = menuBar->addMenu(tr("File"));

	QAction *actionNew = new QAction(tr("New"), this);
    actionNew->setShortcut(QKeySequence("Ctrl+N"));
	actionNew->setIcon(QIcon(":/images/images/filenew.png"));
	menuFile->addAction(actionNew);
	connect(actionNew, SIGNAL(triggered()), this, SLOT(newFile()));

	QAction *actionOpen = new QAction(tr("Open..."), this);
    actionOpen->setShortcut(QKeySequence("Ctrl+O"));
	actionOpen->setIcon(QIcon(":/images/images/fileopen.png"));
	menuFile->addAction(actionOpen);
	connect(actionOpen, SIGNAL(triggered()), this, SLOT(open()));

	menuFile->addMenu(menuRecent);

	menuHelp = menuBar->addMenu(tr("Help"));

	QAction *aboutAction = new QAction(tr("About QTeX..."), this);
	menuHelp->addAction(aboutAction);
	connect(aboutAction, SIGNAL(triggered()), qApp, SLOT(about()));
#endif
}

void QTeXApp::about()
{
   QMessageBox::about(activeWindow(), tr("About QTeX..."),
			tr("<p>QTeX is a simple environment for editing, "
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
	TeXDocument *doc = TeXDocument::findDocument(fileName);
	if (doc) {
		doc->show();
		doc->raise();
		doc->activateWindow();
		return;
	}

	doc = new TeXDocument(fileName);
	if (doc->untitled()) {
		delete doc;
		return;
	}
	doc->show();
}

void QTeXApp::preferences()
{
}

void QTeXApp::updateRecentFileActions()
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();

	int numRecentFiles = qMin(files.size(), kMaxRecentFiles);

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = TeXDocument::strippedName(files[i]);
		recentFileActs[i]->setText(text);
		recentFileActs[i]->setData(files[i]);
		recentFileActs[i]->setVisible(true);
	}
	for (int j = numRecentFiles; j < kMaxRecentFiles; ++j)
		recentFileActs[j]->setVisible(false);
}

