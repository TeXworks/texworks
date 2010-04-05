/*
 This is part of TeXworks, an environment for working with TeX documents
 Copyright (C) 2007-2010  Jonathan Kew
 
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

#ifndef TWScriptable_H
#define TWScriptable_H

#include "TWScript.h"

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QProcess>

class QMenu;
class QAction;
class QSignalMapper;

class TWScriptList : public QObject
{
	Q_OBJECT

public:
	TWScriptList()
	{ }
	
	TWScriptList(const TWScriptList& orig)
	: QObject(orig.parent())
	, name(orig.name)
	{ }
	
	TWScriptList(QObject* parent, const QString& str = QString())
	: QObject(parent), name(str)
	{ }
	
	const QString& getName() const { return name; }

private:
	QString name; // name of the folder/submenu
	// scripts and subfolders are stored as children of the QObject
};

class JSScript : public TWScript
{
	Q_OBJECT
	Q_INTERFACES(TWScript)
	
public:
	JSScript(TWScriptLanguageInterface* interface, const QString& filename)
		: TWScript(interface, filename) { }
		
	virtual bool parseHeader() { return doParseHeader("", "", "//"); };

protected:
	virtual bool execute(TWScriptAPI *tw) const;
};

// for JSScript, we provide a plugin-like factory, but it's actually compiled
// and linked directly with the main application (at least for now)
class JSScriptInterface : public QObject, public TWScriptLanguageInterface
{
	Q_OBJECT
	Q_INTERFACES(TWScriptLanguageInterface)
	
public:
	JSScriptInterface() {};
	virtual ~JSScriptInterface() {};

	virtual TWScript* newScript(const QString& fileName);

	virtual QString scriptLanguageName() const { return QString("QtScript"); }
	virtual QString scriptLanguageURL() const { return QString("http://doc.trolltech.com/4.5/qtscript.html"); }
	virtual bool canHandleFile(const QFileInfo& fileInfo) const { return fileInfo.suffix() == QString("js"); }
};

class TWScriptManager
{
public:
	TWScriptManager();
	virtual ~TWScriptManager() {};
	
	bool addScript(QObject* scriptList, TWScript* script);
	void addScriptsInDirectory(const QDir& dir, const QStringList& disabled) {
		addScriptsInDirectory(&m_Scripts, &m_Hooks, dir, disabled);
	}
	void clear();
		
	TWScriptList* getScripts() { return &m_Scripts; }
	TWScriptList* getHookScripts() { return &m_Hooks; }
	QList<TWScript*> getHookScripts(const QString& hook) const;

	const QList<TWScriptLanguageInterface*>& languages() const { return scriptLanguages; }

	void loadScripts();
	void saveDisabledList();

protected:
	void addScriptsInDirectory(TWScriptList *scriptList,
							   TWScriptList *hookList,
							   const QDir& dir,
							   const QStringList& disabled);
	void loadPlugins();
	
private:
	TWScriptList m_Scripts; // hierarchical list of standalone scripts
	TWScriptList m_Hooks; // hierarchical list of hook scripts

	QList<TWScriptLanguageInterface*> scriptLanguages;
};

// parent class for document windows that handle a Scripts menu
// (i.e. both the source and PDF window types)
class TWScriptable : public QMainWindow
{
	Q_OBJECT

public:
	TWScriptable();
	virtual ~TWScriptable() { }
	
public slots:
	void updateScriptsMenu();
	void runScript(QObject * script, TWScript::ScriptType scriptType = TWScript::ScriptStandalone);
	void runHooks(const QString& hookName);
	
private slots:
	void doManageScripts();
	void doAboutScripts();

protected:
	void initScriptable(QMenu* scriptsMenu,
						QAction* aboutScriptsAction,
						QAction* manageScriptsAction,
						QAction* updateScriptsAction,
						QAction* showScriptsFolderAction);

	int addScriptsToMenu(QMenu *menu, TWScriptList *scripts);

private:
	QMenu* scriptsMenu;
	QSignalMapper* scriptMapper;
	int staticScriptMenuItemCount;
};


class TWSystemCmd : public QProcess {
	Q_OBJECT
	
public:
	TWSystemCmd(QObject* parent, bool isOutputWanted = true)
		: QProcess(parent), wantOutput(isOutputWanted) {}
	virtual ~TWSystemCmd() {}
	
public slots:
	void processError(QProcess::ProcessError error) {
		if (wantOutput)
			result = tr("ERROR: failure code %1").arg(error);
		deleteLater();
	}
	void processFinished(int exitCode, QProcess::ExitStatus exitStatus) {
		if (wantOutput) {
			if (exitStatus == QProcess::NormalExit) {
				if (bytesAvailable() > 0) {
					QByteArray ba = readAllStandardOutput();
					result += QString::fromLocal8Bit(ba);
				}
			}
			else {
				result = tr("ERROR: exit code %1").arg(exitCode);
			}
		}
		deleteLater();
	}
	void processOutput() {
		if (wantOutput && bytesAvailable() > 0) {
			QByteArray ba = readAllStandardOutput();
			result += QString::fromLocal8Bit(ba);
		}
	}

	QString getResult() { return result; }
	
private:
	bool wantOutput;
	QString result;
};

#endif
