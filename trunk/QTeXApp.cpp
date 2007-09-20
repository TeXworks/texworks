#include "QTeXApp.h"
#include "TeXDocument.h"
#include "PDFDocument.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QMenuBar>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QStringList>
#include <QKeySequence>

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
	updateRecentFileActions(this, recentFileActions, menuRecent);	
#endif
	emit recentFileActionsChanged();
}

void QTeXApp::updateWindowMenus()
{
	emit windowListChanged();
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
		QString text = QTeXApp::strippedName(files[i]);
		actions[i]->setText(text);
		actions[i]->setData(files[i]);
		actions[i]->setVisible(true);
	}
}

void QTeXApp::updateWindowMenu(QWidget *window, QMenu *menu) /* static */
{
	// shorten the menu by removing everything from the first "selectWindow" action onwards
	QList<QAction*> actions = menu->actions();
	for (QList<QAction*>::iterator i = actions.begin(); i != actions.end(); ++i) {
		SelWinAction *selWin = qobject_cast<SelWinAction*>(*i);
		if (selWin)
			menu->removeAction(*i);
	}
	while (!menu->actions().isEmpty() && menu->actions().last()->isSeparator())
		menu->removeAction(menu->actions().last());
	
	// append an item for each TeXDocument
	bool first = true;
	foreach (TeXDocument *texDoc, TeXDocument::documentList()) {
		if (first && !menu->actions().isEmpty())
			menu->addSeparator();
		first = false;
		SelWinAction *selWin = new SelWinAction(menu, texDoc->fileName());
		if (texDoc == qobject_cast<TeXDocument*>(window)) {
			selWin->setCheckable(true);
			selWin->setChecked(true);
		}
		connect(selWin, SIGNAL(triggered()), texDoc, SLOT(selectWindow()));
		menu->addAction(selWin);
	}
	
	// append an item for each PDFDocument
	first = true;
	foreach (PDFDocument *pdfDoc, PDFDocument::documentList()) {
		if (first && !menu->actions().isEmpty())
			menu->addSeparator();
		first = false;
		SelWinAction *selWin = new SelWinAction(menu, pdfDoc->fileName());
		if (pdfDoc == qobject_cast<PDFDocument*>(window)) {
			selWin->setCheckable(true);
			selWin->setChecked(true);
		}
		connect(selWin, SIGNAL(triggered()), pdfDoc, SLOT(selectWindow()));
		menu->addAction(selWin);
	}
}

QString QTeXApp::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

SelWinAction::SelWinAction(QObject *parent, const QString &fileName)
	: QAction(parent)
{
	setText(QTeXApp::strippedName(fileName));
	setData(fileName);
}
