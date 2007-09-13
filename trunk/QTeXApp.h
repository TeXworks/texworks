#ifndef QTeXApp_H
#define QTeXApp_H

#include <QApplication>

class QString;
class QAction;
class QMenu;
class QMenuBar;

const int kMaxRecentFiles = 10;

class QTeXApp : public QApplication
{
	Q_OBJECT

public:
	QTeXApp(int argc, char *argv[]);

	QMenu *getRecentFilesMenu()
		{ return menuRecent; }

	void updateRecentFileActions();

#ifdef Q_WS_MAC
	QMenuBar *menuBar;
	QMenu *menuFile;
	QMenu *menuHelp;
#endif

private slots:
	void about();
	void newFile();
	void open();
	void openRecentFile();
	void preferences();

private:
	void init();
	void open(const QString &fileName);

	QAction *recentFileActs[kMaxRecentFiles];
	QMenu *menuRecent;
};

#endif
