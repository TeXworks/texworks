#ifndef QTeXApp_H
#define QTeXApp_H

#include <QApplication>
#include <QList>
#include <QAction>

class QString;
class QMenu;
class QMenuBar;

/* general constants used by multiple document types */
const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

class QTeXApp : public QApplication
{
	Q_OBJECT
    Q_PROPERTY(int maxRecentFiles READ maxRecentFiles WRITE setMaxRecentFiles)

public:
	QTeXApp(int argc, char *argv[]);

	/* static method to actually perform the updates to a menu; used by the documents to update their own menus */
	static void updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu);

	/* update the SelWinActions in a menu, used by the documents */
	static void updateWindowMenu(QWidget *window, QMenu *menu);

	/* return just the filename from a full pathname, suitable for UI display */
	static QString strippedName(const QString &fullFileName);

	int maxRecentFiles() const;
	void setMaxRecentFiles(int value);

#ifdef Q_WS_MAC
	/* on the Mac only, we have a top-level app menu bar, including its own copy of the recent files menu */
	QMenu *getRecentFilesMenu()
		{ return menuRecent; }

private:
	QMenuBar *menuBar;
	QMenu *menuFile;
	QMenu *menuHelp;
	QList<QAction*> recentFileActions;
	QMenu *menuRecent;
#endif

public slots:
	/* called by documents when they load a file */
	void updateRecentFileActions();

	/* called by windows when the open/close/change name */
	void updateWindowMenus();

signals:
	/* emitted in response to updateRecentFileActions(); documents can listen to this if they have a recent files menu */
	void recentFileActionsChanged();
	/* emitted when the window list may have changed, so documents can update their window menu */
	void windowListChanged();

private slots:
	void about();
	void newFile();
	void open();
	void openRecentFile();
	void preferences();

private:
	void init();
	void open(const QString &fileName);
	
	int f_maxRecentFiles;
};

class SelWinAction : public QAction
{
	Q_OBJECT
	
public:
	SelWinAction(QObject *parent, const QString &fileName);
};

#endif
