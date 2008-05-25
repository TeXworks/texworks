/*
    This is part of TeXworks, an environment for working with TeX documents
    Copyright (C) 2007-08  Jonathan Kew

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TWApp_H
#define TWApp_H

#include <QApplication>
#include <QList>
#include <QAction>

#include "TWUtils.h"

class QString;
class QMenu;
class QMenuBar;

// general constants used by multiple document types
const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

class TWApp : public QApplication
{
	Q_OBJECT

public:
	TWApp(int &argc, char **argv);

	void launchAction();

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

	static TWApp *instance();

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
	void newFromTemplate();
	void open();
	void openRecentFile();
	void preferences();

	void stackWindows();
	void tileWindows();
	void tileTwoWindows();
	
	void syncFromSource(const QString& sourceFile, int lineNo);


protected:
	bool event(QEvent *);

private:
	void init();
	
	int recentFilesLimit;

	QTextCodec *defaultCodec;

	QStringList *binaryPaths;
	QList<Engine> *engineList;
	int defaultEngineIndex;

	static TWApp *theAppInstance;
};

inline TWApp *TWApp::instance()
{
	return theAppInstance;
}

#endif
