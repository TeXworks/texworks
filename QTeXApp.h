#ifndef QTeXApp_H
#define QTeXApp_H

#include <QApplication>
#include <QList>
#include <QAction>

#include "QTeXUtils.h"

#define TEXWORKS_NAME "TeXworks" /* app name, for use in menus, messages, etc */

class QString;
class QMenu;
class QMenuBar;

// general constants used by multiple document types
const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

class QTeXApp : public QApplication
{
	Q_OBJECT

public:
	QTeXApp(int &argc, char **argv);

	int maxRecentFiles() const;
	void setMaxRecentFiles(int value);

	void setBinaryPaths(const QStringList& paths);
	void setEngineList(const QList<Engine>& engines);

	void open(const QString &fileName);

	const QStringList getBinaryPaths();
	const QList<Engine> getEngineList();

	const Engine getNamedEngine(const QString& name);
	const Engine getDefaultEngine();
	void setDefaultEngine(const QString& name);

	void setDefaultPaths();
	void setDefaultEngineList();
	
	QTextCodec *getDefaultCodec();
	void setDefaultCodec(QTextCodec *codec);

#ifdef Q_WS_MAC
private:
	// on the Mac only, we have a top-level app menu bar, including its own copy of the recent files menu
	QMenuBar *menuBar;
	QMenu *menuFile;
	QMenu *menuHelp;
	QList<QAction*> recentFileActions;
	QMenu *menuRecent;
#endif

public slots:
	// called by documents when they load a file
	void updateRecentFileActions();

	// called by windows when they open/close/change name
	void updateWindowMenus();

	void stackWindows();
	void tileWindows();
	void tileTwoWindows();
	
	void syncFromSource(const QString& sourceFile, int lineNo);

signals:
	// emitted in response to updateRecentFileActions(); documents can listen to this if they have a recent files menu
	void recentFileActionsChanged();

	// emitted when the window list may have changed, so documents can update their window menu
	void windowListChanged();
	
	// emitted when the engine list is changed from Preferences, so docs can update their menus
	void engineListChanged();
	
	void syncPdf(const QString& sourceFile, int lineNo);

private slots:
	void about();
	void newFile();
	void open();
	void openRecentFile();
	void preferences();

protected:
	bool event(QEvent *);

private:
	void init();
	
	int recentFilesLimit;

	QTextCodec *defaultCodec;

	QStringList *binaryPaths;
	QList<Engine> *engineList;
	int defaultEngineIndex;
};

#endif
