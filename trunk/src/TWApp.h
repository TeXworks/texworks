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

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the author,
	see <http://texworks.org/>.
*/

#ifndef TWApp_H
#define TWApp_H

#include <QApplication>
#include <QList>
#include <QAction>
#include <QSettings>

#include "TWUtils.h"

class QString;
class QMenu;
class QMenuBar;

// general constants used by multiple document types
const int kStatusMessageDuration = 3000;
const int kNewWindowOffset = 32;

#ifdef Q_WS_WIN // for communication with the original instance
#define _WIN32_WINNT			0x0500	// for HWND_MESSAGE
#include <windows.h>
#define TW_HIDDEN_WINDOW_CLASS	TEXWORKS_NAME ":MessageTarget"
#define TW_OPEN_FILE_MSG		(('T' << 8) + 'W')	// just a small sanity check for the receiver
#endif

#define QSETTINGS_OBJECT(s) \
			QSettings s(TWApp::instance()->getSettingsFormat(), QSettings::UserScope, \
						TWApp::instance()->organizationName(), TWApp::instance()->applicationName())

class TWApp : public QApplication
{
	Q_OBJECT

public:
	TWApp(int &argc, char **argv);

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

	void openUrl(const QString& urlString);

	QSettings::Format getSettingsFormat() const { return settingsFormat; }
	void setSettingsFormat(QSettings::Format fmt) { settingsFormat = fmt; }
	
	static TWApp *instance();
	
	QString getPortableLibPath() const { return portableLibPath; }

#ifdef Q_WS_WIN
	void createMessageTarget(QWidget* aWindow);
#endif
#ifdef Q_WS_X11
	void bringToFront();
#endif

#ifdef Q_WS_MAC
private:
	// on the Mac only, we have a top-level app menu bar, including its own copy of the recent files menu
	QMenuBar *menuBar;

	QMenu *menuFile;
	QAction *actionNew;
	QAction *actionNew_from_Template;
	QAction *actionOpen;
	QAction *actionPreferences;

	QMenu *menuRecent;
	QList<QAction*> recentFileActions;

	QMenu *menuHelp;
	QAction *aboutAction;
	QAction *homePageAction;
	QAction *mailingListAction;
#endif

public slots:
	// called by documents when they load a file
	void updateRecentFileActions();

	// called by windows when they open/close/change name
	void updateWindowMenus();

	// called once when the app is first launched
	void launchAction();

	void activatedWindow(QWidget* theWindow);

	void goToHomePage();
	void writeToMailingList();

	void applyTranslation(const QString& locale);
	
	void maybeQuit();

signals:
	// emitted in response to updateRecentFileActions(); documents can listen to this if they have a recent files menu
	void recentFileActionsChanged();

	// emitted when the window list may have changed, so documents can update their window menu
	void windowListChanged();
	
	// emitted when the engine list is changed from Preferences, so docs can update their menus
	void engineListChanged();
	
	void syncPdf(const QString& sourceFile, int lineNo);

	void hideFloatersExcept(QWidget* theWindow);

	void updatedTranslators();

private slots:
	void about();
	void newFile();
	void newFromTemplate();
	void open();
	void openRecentFile();
	void preferences();

	void stackWindows();
	void tileWindows();
	
	void syncFromSource(const QString& sourceFile, int lineNo);

	void changeLanguage();

protected:
	virtual bool event(QEvent *);

private:
	void init();
	
	void arrangeWindows(TWUtils::WindowArrangementFunction func);

	int recentFilesLimit;

	QTextCodec *defaultCodec;

	QStringList *binaryPaths;
	QList<Engine> *engineList;
	int defaultEngineIndex;
	QString portableLibPath;

	QList<QTranslator*> translators;

	QSettings::Format settingsFormat;
	
#ifdef Q_WS_WIN
	HWND messageTargetWindow;
#endif

	static TWApp *theAppInstance;
};

inline TWApp *TWApp::instance()
{
	return theAppInstance;
}

#ifdef Q_WS_X11
#include <QtDBus>

#define TW_SERVICE_NAME 	"org.tug.texworks.application"
#define TW_APP_PATH		"/org/tug/texworks/application"
#define TW_INTERFACE_NAME	"org.tug.texworks.application"

class TWAdaptor: public QDBusAbstractAdaptor
{
	Q_OBJECT
	Q_CLASSINFO("D-Bus Interface", "org.tug.texworks.application") // using the #define here fails :(

private:
	TWApp *app;

public:
	TWAdaptor(TWApp *application)
		: QDBusAbstractAdaptor(application), app(application)
		{ }
	
public slots:
	Q_NOREPLY void openFile(const QString& fileName)
		{ app->open(fileName); }
	Q_NOREPLY void bringToFront()
		{ app->bringToFront(); }
};
#endif	// Q_WS_X11

#endif	// TWApp_H

