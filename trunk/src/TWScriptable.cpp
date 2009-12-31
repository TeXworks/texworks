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

#include "TWScriptable.h"
#include "TWApp.h"

#include <QSignalMapper>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QStatusBar>
#include <QtDebug>
#include <QtScript>
#if QT_VERSION >= 0x040500
#include <QtScriptTools>
#endif

bool JSScript::parseHeader()
{
	QString line, key, value;
	QFile file(m_Filename);
	
	clearHeaderData();
	
	if (!file.exists() || !file.open(QIODevice::ReadOnly))
		return false;
	
	// skip any empty lines
	QTextStream s(&file);
	while (!s.atEnd()) {
		line = s.readLine().trimmed();
		if (!line.isEmpty())
			break;
	}
	
	// is this a valid TW script?
	if (!line.startsWith("//"))
		return false;
	line = line.mid(2).trimmed();
	if (!line.startsWith("TeXworksScript"))
		return false;
	
	// read the header lines
	while (!s.atEnd()) {
		line = s.readLine().trimmed();
		
		// have we reached the end?
		if (!line.startsWith("//"))
			break;
		
		line = line.mid(2).trimmed();
		key = line.section(':', 0, 0).trimmed();
		value = line.section(':', 1).trimmed();
		
		if (key == "Title") m_Title = value;
		else if (key == "Description") m_Description = value;
		else if (key == "Author") m_Author = value;
		else if (key == "Version") m_Version = value;
		else if (key == "Script-Type") {
			if (value == "hook") m_Type = ScriptHook;
			else if (value == "standalone") m_Type = ScriptStandalone;
			else m_Type = ScriptUnknown;
		}
		else if (key == "Hook") m_Hook = value;
		else if (key == "Context") m_Context = value;
		else if (key == "Shortcut") m_KeySequence = QKeySequence(value);
	}
	
	return m_Type != ScriptUnknown && !m_Title.isEmpty();
}

static
QVariant convertValue(const QScriptValue& value)
{
	if (value.isArray()) {
		QVariantList lst;
		int len = value.property("length").toUInt32();
		for (int i = 0; i < len; ++i) {
			QScriptValue p = value.property(i);
			lst.append(convertValue(p));
		}
		return lst;
	}
	else
		return value.toVariant();
}

bool JSScript::run(QObject *context, QVariant& result) const
{
	QFile scriptFile(m_Filename);
	if (!scriptFile.open(QIODevice::ReadOnly)) {
		// handle error
		return false;
	}
	QTextStream stream(&scriptFile);
	QString contents = stream.readAll();
	scriptFile.close();
	
	QScriptEngine engine;
	QScriptValue targetObject = engine.newQObject(context);
	engine.globalObject().setProperty("target", targetObject);
	
	QScriptValue appObject = engine.newQObject(TWApp::instance());
	engine.globalObject().setProperty("app", appObject);
	
	QScriptValue val;

#if QT_VERSION >= 0x040500
	QSETTINGS_OBJECT(settings);
	if (settings.value("scriptDebugger", false).toBool()) {
		QScriptEngineDebugger debugger;
		debugger.attachTo(&engine);
		val = engine.evaluate(contents, m_Filename);
	}
	else {
		val = engine.evaluate(contents, m_Filename);
	}
#else
	val = engine.evaluate(contents, m_Filename);
#endif

	if (engine.hasUncaughtException()) {
		result = engine.uncaughtException().toString();
		return false;
	}
	else {
		if (!val.isUndefined()) {
			result = convertValue(val);
		}
		return true;
	}
}

void TWScriptManager::clear()
{
	foreach (QObject *s, m_Scripts.children())
		delete s;

	foreach (QObject *s, m_Hooks.children())
		delete s;
}

bool TWScriptManager::addScript(QObject* scriptList, TWScript* script)
{
	foreach (QObject* obj, scriptList->children()) {
		printf("obj class = %s\n", obj->metaObject()->className());
		printf("is a script? %d\n", obj->inherits("TWScript"));
		TWScript *s = qobject_cast<TWScript*>(obj);
		if (!s)
			continue;
		if (*s == *script)
			return false;
	}
	
	script->setParent(scriptList);
	return true;
}

int TWScriptManager::addScriptsInDirectory(TWScriptList *scriptList, const QDir& dir)
{
	int num = 0;
	
	foreach (const QFileInfo& info,
			 dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable, QDir::DirsFirst)) {
		if (info.isDir()) {
			TWScriptList *sublist = new TWScriptList(scriptList, info.fileName());
			if (addScriptsInDirectory(sublist, info.absoluteFilePath()) == 0)
				delete sublist;
			else
				++num;
			continue;
		}
		QString suffix = info.suffix();
		if (suffix == "js") {
			TWScript *script = new JSScript(info.absoluteFilePath());
			if (script && script->parseHeader()) {
				if (script->getType() == TWScript::ScriptHook) {
					if (!addScript(&m_Hooks, script))
						delete script;
				}
				else {
					if (addScript(scriptList, script))
						++num;
					else
						delete script;
				}
			}
			else
				delete script;
			continue;
		}
		// add other types of script here
	}
	
	return num;
}

QList<TWScript*> TWScriptManager::getHookScripts(const QString& hook) const
{
	QList<TWScript*> result;
	
	foreach (QObject *obj, m_Hooks.children()) {
		TWScript *script = qobject_cast<TWScript*>(obj);
		if (!script)
			continue;
		if (script->getHook().compare(hook, Qt::CaseInsensitive) == 0)
			result.append(script);
	}
	return result;
}

TWScriptable::TWScriptable()
	: QMainWindow(),
	  scriptsMenu(NULL),
	  staticScriptMenuItemCount(0)
{
}

void
TWScriptable::initScriptable(QMenu* theScriptsMenu,
							 QAction* manageScriptsAction,
							 QAction* updateScriptsAction,
							 QAction* showScriptsFolderAction)
{
	scriptsMenu = theScriptsMenu;
	connect(manageScriptsAction, SIGNAL(triggered()), this, SLOT(doManageScriptsDialog()));
	connect(updateScriptsAction, SIGNAL(triggered()), TWApp::instance(), SLOT(updateScriptsList()));
	connect(showScriptsFolderAction, SIGNAL(triggered()), TWApp::instance(), SLOT(showScriptsFolder()));
	scriptMapper = new QSignalMapper(this);
	connect(scriptMapper, SIGNAL(mapped(QObject*)), this, SLOT(runScript(QObject*)));
	staticScriptMenuItemCount = scriptsMenu->actions().count();
	
	updateScriptsMenu();
}

void
TWScriptable::updateScriptsMenu()
{
	QList<QAction*> actions = scriptsMenu->actions();
	for (int i = staticScriptMenuItemCount; i < actions.count(); ++i) {
		scriptsMenu->removeAction(actions[i]);
		delete actions[i];
	}
	
	addScriptsToMenu(scriptsMenu, TWApp::instance()->getScriptManager().getScripts());
}

int
TWScriptable::addScriptsToMenu(QMenu *menu, TWScriptList *scripts)
{
	int count = 0;
	foreach (QObject *obj, scripts->children()) {
		TWScript *script = qobject_cast<TWScript*>(obj);
		if (script) {
			if (script->getContext().isEmpty() || script->getContext() == metaObject()->className()) {
				QAction *a = menu->addAction(script->getTitle());
				if (!script->getKeySequence().isEmpty())
					a->setShortcut(script->getKeySequence());
				// give the action an object name so it could possibly included in the
				// customization process of keyboard shortcuts in the future
				a->setObjectName(QString("Script: %1").arg(script->getTitle()));
				a->setStatusTip(script->getDescription());
				scriptMapper->setMapping(a, script);
				connect(a, SIGNAL(triggered()), scriptMapper, SLOT(map()));
				++count;
			}
			continue;
		}
		TWScriptList *list = qobject_cast<TWScriptList*>(obj);
		if (list) {
			QMenu *m = menu->addMenu(list->getName());
			if (addScriptsToMenu(m, list) == 0)
				menu->removeAction(m->menuAction());
		}
	}
	return count;
}

void
TWScriptable::runScript(QObject* script, TWScript::ScriptType scriptType)
{
	TWScript * s = qobject_cast<TWScript*>(script);
	if (!s || s->getType() != scriptType)
		return;
	
	QVariant result;
	bool success = s->run(this, result);
	if (success) {
		if (!result.isNull()) {
			if (scriptType == TWScript::ScriptHook)
				statusBar()->showMessage(tr("Script \"%1\": %2").arg(s->getTitle()).arg(result.toString()), kStatusMessageDuration);
			else
				QMessageBox::information(this, tr("Script result"), result.toString(), QMessageBox::Ok, QMessageBox::Ok);
		}
	}
	else {
		if (result.isNull())
			result = tr("unknown error");
		statusBar()->showMessage(tr("Script \"%1\": %2").arg(s->getTitle()).arg(result.toString()));
	}
}

void
TWScriptable::runHooks(const QString& hookName)
{
	foreach (TWScript *s, TWApp::instance()->getScriptManager().getHookScripts(hookName)) {
		runScript(s, TWScript::ScriptHook);
	}
}

void
TWScriptable::doManageScriptsDialog()
{
}
