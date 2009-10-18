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

#include <QMainWindow>
#include <QList>
#include <QString>
#include <QFileInfo>
#include <QDir>
#include <QProcess>

class QMenu;
class QAction;
class QSignalMapper;

// must be derived from QObject to enable interaction with e.g. menus
class TWScript : public QObject
{
	Q_OBJECT
	
public:
	enum ScriptType { ScriptUnknown, ScriptHook, ScriptStandalone };
	enum ScriptLanguage { LanguageQtScript, LanguageLua };
	
	virtual ~TWScript() { }
	
	ScriptType getType() const { return m_Type; }
	QString getFilename() const { return m_Filename; }
	QString getTitle() const { return m_Title; }
	QString getDescription() const { return m_Description; }
	QString getAuthor() const { return m_Author; }
	QString getVersion() const { return m_Version; }
	QString getHook() const { return m_Hook; }
	
	bool setFile(QString filename);

	virtual ScriptLanguage getLanguage() const = 0;
	virtual bool parseHeader() = 0;
	virtual bool run(QObject *context, QVariant& result) const = 0;
	
	bool operator==(const TWScript& s) const { return QFileInfo(m_Filename) == QFileInfo(s.m_Filename); }
	
protected:
	TWScript(const QString& filename) : m_Filename(filename) { }
	void clearHeaderData();
	
	QString m_Filename;
	
	ScriptType m_Type;
	
	QString m_Title;
	QString m_Description;
	QString m_Author;
	QString m_Version;
	QString m_Hook;
};

class JSScript : public TWScript
{
	Q_OBJECT
		
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
	virtual ~TWScriptManager() { clear(); }
	
	bool addScript(TWScript* script);
	int addScriptsInDirectory(const QDir& dir);
	void clear();
	
	QList<TWScript*> getScripts() const { return m_Scripts; }
	QList<TWScript*> getHookScripts(const QString& hook) const;
	
private:
	QList<TWScript*> m_Scripts;
	QList<TWScript*> m_Hooks;
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
	void runScript(QObject * script);
	void runHooks(const QString& hookName);
	
private slots:
	void doManageScriptsDialog();

protected:
	void initScriptable(QMenu* scriptsMenu,
						QAction* manageScriptsAction,
						QAction* updateScriptsAction,
						QAction* showScriptsFolderAction);

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
