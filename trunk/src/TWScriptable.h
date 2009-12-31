/*
 This is part of TeXworks, an environment for working with TeX documents
 Copyright (C) 2007-09  Jonathan Kew
 
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
	JSScript(const QString& filename) : TWScript(filename) { }
	
	virtual ScriptLanguage getLanguage() const { return LanguageQtScript; }
	
	virtual bool parseHeader();
	virtual bool run(QObject *context, QVariant& result) const;
};

class TWScriptManager
{
public:
	TWScriptManager() { }
	virtual ~TWScriptManager() { }
	
	bool addScript(QObject* scriptList, TWScript* script);
	int addScriptsInDirectory(const QDir& dir) {
		return addScriptsInDirectory(&m_Scripts, dir);
	}
	void clear();
	
	TWScriptList* getScripts() { return &m_Scripts; }
	QList<TWScript*> getHookScripts(const QString& hook) const;

protected:
	int addScriptsInDirectory(TWScriptList *scriptList, const QDir& dir);
	
private:
	TWScriptList m_Scripts; // hierarchical list of standalone scripts
	TWScriptList m_Hooks; // flat list of hook scripts (not shown in menus)
};


// parent class for document windows that handle a Scripts menu
// (i.e. both the source and PDF window types)
class TWScriptable : public QMainWindow
{
	Q_OBJECT

public:
	TWScriptable();
	virtual ~TWScriptable() { }
	
	void updateScriptsMenu();

public slots:
	void runScript(QObject * script, TWScript::ScriptType scriptType = TWScript::ScriptStandalone);
	void runHooks(const QString& hookName);
	
private slots:
	void doManageScriptsDialog();

protected:
	void initScriptable(QMenu* scriptsMenu,
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
