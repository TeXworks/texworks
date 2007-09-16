#ifndef QTeXApp_H
#define QTeXApp_H

#include <QApplication>
#include <QList>

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
	static void updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu);

#ifdef Q_WS_MAC
private:
	QMenuBar *menuBar;
	QMenu *menuFile;
	QMenu *menuHelp;
	QList<QAction*> recentFileActions;
	QMenu *menuRecent;
#endif

signals:
	void recentFileActionsChanged();

private slots:
	void about();
	void newFile();
	void open();
	void openRecentFile();
	void preferences();

private:
	void init();
	void open(const QString &fileName);
};

#endif
