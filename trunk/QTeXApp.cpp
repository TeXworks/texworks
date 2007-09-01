#include "QTeXApp.h"

#include "TeXDocument.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QString>
#include <QMenuBar>
#include <QMenu>
#include <QAction>

QTeXApp::QTeXApp(int argc, char *argv[])
	: QApplication(argc, argv)
{
	init();
}

void
QTeXApp::init()
{
	setOrganizationName("TUG");
	setOrganizationDomain("tug.org");
	setApplicationName("QTeX");
#ifdef Q_WS_MAC
	setQuitOnLastWindowClosed(false);
#endif
}

void
QTeXApp::about()
{
   QMessageBox::about(activeWindow(), tr("About QTeX..."),
			tr("<p>QTeX is a simple environment for editing, "
			    "typesetting, and previewing TeX documents.</p>"
				"<p>Distributed under the GNU General Public License, version 2.</p>"
				"<p>&#xA9; 2007 Jonathan Kew.</p>"
				"<p>Built using the Qt toolkit from <a href=\"http://trolltech.com/\">Trolltech</a>.</p>"
				));
}


