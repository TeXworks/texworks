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
	setApplicationName("TeXWorks");

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
	updateRecentFileActions(this, recentFileActions, menuRecent);	
	emit recentFileActionsChanged();
}

void QTeXApp::updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu) /* static */
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	int numRecentFiles = files.size();

	while (actions.size() < numRecentFiles) {
		QAction *act = new QAction(parent);
		act->setVisible(false);
		connect(act, SIGNAL(triggered()), qApp, SLOT(openRecentFile()));
		actions.append(act);
		menu->addAction(act);
	}

	while (actions.size() > numRecentFiles) {
		QAction *act = actions.takeLast();
		delete act;
	}

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = TeXDocument::strippedName(files[i]);
		actions[i]->setText(text);
		actions[i]->setData(files[i]);
		actions[i]->setVisible(true);
	}
}
