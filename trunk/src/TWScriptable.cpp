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

#include "TWScriptable.h"
#include "ScriptManager.h"
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

#ifdef STATIC_SCRIPTING_PLUGINS
#include <QtPlugin>
Q_IMPORT_PLUGIN(TWLuaPlugin)
Q_IMPORT_PLUGIN(TWPythonPlugin)
#endif

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

bool JSScript::execute(TWInterface *tw) const
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
	QScriptValue twObject = engine.newQObject(tw);
	engine.globalObject().setProperty("TW", twObject);
	
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
		tw->SetResult(engine.uncaughtException().toString());
		return false;
	}
	else {
		if (!val.isUndefined()) {
			tw->SetResult(convertValue(val));
		}
		return true;
	}
}

TWScript* JSScriptInterface::newScript(const QString& fileName)
{
	return new JSScript(this, fileName);
}

TWScriptManager::TWScriptManager()
{
	loadPlugins();
	loadScripts();
}

void
TWScriptManager::saveDisabledList()
{
	QDir scriptRoot(TWUtils::getLibraryPath("scripts"));
	QStringList disabled;

	QList<QObject*> list = m_Scripts.findChildren<QObject*>();
	foreach (QObject* i, list) {
		TWScript * s = qobject_cast<TWScript*>(i);
		if (!s || s->isEnabled())
			continue;
		disabled << scriptRoot.relativeFilePath(s->getFilename());
	}
	list = m_Hooks.findChildren<QObject*>();
	foreach (QObject* i, list) {
		TWScript * s = qobject_cast<TWScript*>(i);
		if (!s || s->isEnabled())
			continue;
		disabled << scriptRoot.relativeFilePath(s->getFilename());
	}
	
	QSETTINGS_OBJECT(settings);
	settings.setValue("disabledScripts", disabled);
}

void TWScriptManager::loadPlugins()
{
	// the JSScript interface isn't really a plugin, but provides the same interface
	scriptLanguages += new JSScriptInterface();
	
	// get any static plugins
	foreach (QObject *plugin, QPluginLoader::staticInstances()) {
		TWScriptLanguageInterface *s = qobject_cast<TWScriptLanguageInterface*>(plugin);
		if (s)
			scriptLanguages += s;
	}

#ifdef TW_PLUGINPATH
	// allow a hard-coded path for distro packagers
	QDir pluginsDir = QDir(TW_PLUGINPATH);
#else
	// find the plugins directory, relative to the executable
	QDir pluginsDir = QDir(qApp->applicationDirPath());
#if defined(Q_OS_WIN)
	if (pluginsDir.dirName().toLower() == "debug" || pluginsDir.dirName().toLower() == "release")
		pluginsDir.cdUp();
#elif defined(Q_OS_MAC) // "plugins" directory is alongside "MacOS" within the package's Contents dir
	if (pluginsDir.dirName() == "MacOS")
		pluginsDir.cdUp();
	if (!pluginsDir.exists("plugins")) { // if not found, try for a dir alongside the .app package
		pluginsDir.cdUp();
		pluginsDir.cdUp();
	}
#endif
	pluginsDir.cd("plugins");
#endif

	// allow an env var to override the default plugin path
	const char* pluginPath = getenv("TW_PLUGINPATH");
	if (pluginPath != NULL)
		pluginsDir.cd(QString(pluginPath));

	foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
		QPluginLoader loader(pluginsDir.absoluteFilePath(fileName));
		QObject *plugin = loader.instance();
		TWScriptLanguageInterface *s = qobject_cast<TWScriptLanguageInterface*>(plugin);
		if (s)
			scriptLanguages += s;
	}
}

void TWScriptManager::loadScripts()
{
	QSETTINGS_OBJECT(settings);
	QStringList disabled = settings.value("disabledScripts", QStringList()).toStringList();
	
	// canonicalize the paths
	QDir scriptsDir(TWUtils::getLibraryPath("scripts"));
	for (int i = 0; i < disabled.size(); ++i)
		disabled[i] = QFileInfo(scriptsDir.absoluteFilePath(disabled[i])).canonicalFilePath();
	
	addScriptsInDirectory(scriptsDir, disabled);
	
	ScriptManager::refreshScriptList();
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
		TWScript *s = qobject_cast<TWScript*>(obj);
		if (!s)
			continue;
		if (*s == *script)
			return false;
	}
	
	script->setParent(scriptList);
	return true;
}

void TWScriptManager::addScriptsInDirectory(TWScriptList *scriptList,
											TWScriptList *hookList,
											const QDir& dir,
											const QStringList& disabled)
{
	foreach (const QFileInfo& info,
			 dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable, QDir::DirsLast)) {
		if (info.isDir()) {
			TWScriptList *subScriptList = new TWScriptList(scriptList, info.fileName());
			TWScriptList *subHookList = new TWScriptList(hookList, info.fileName());
			addScriptsInDirectory(subScriptList, subHookList, info.absoluteFilePath(), disabled);
			if (subScriptList->children().isEmpty())
				delete subScriptList;
			if (subHookList->children().isEmpty())
				delete subHookList;
			continue;
		}

		foreach (TWScriptLanguageInterface* i, scriptLanguages) {
			if (!i->canHandleFile(info))
				continue;
			TWScript *script = i->newScript(info.absoluteFilePath());
			if (script) {
				if (disabled.contains(info.canonicalFilePath()))
					script->setEnabled(false);
				script->parseHeader();
				switch (script->getType()) {
					case TWScript::ScriptHook:
						if (!addScript(hookList, script))
							delete script;
						break;

					case TWScript::ScriptStandalone:
						if (!addScript(scriptList, script))
							delete script;
						break;
					
					default: // must be unknown/invalid
						delete script;
						break;
				}
				break;
			}
		}
	}
}

QList<TWScript*> TWScriptManager::getHookScripts(const QString& hook) const
{
	QList<TWScript*> result;
	
	foreach (QObject *obj, m_Hooks.findChildren<QObject*>()) {
		TWScript *script = qobject_cast<TWScript*>(obj);
		if (!script)
			continue;
		if (!script->isEnabled())
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
							 QAction* aboutScriptsAction,
							 QAction* manageScriptsAction,
							 QAction* updateScriptsAction,
							 QAction* showScriptsFolderAction)
{
	scriptsMenu = theScriptsMenu;
	connect(aboutScriptsAction, SIGNAL(triggered()), this, SLOT(doAboutScripts()));
	connect(manageScriptsAction, SIGNAL(triggered()), this, SLOT(doManageScripts()));
	connect(updateScriptsAction, SIGNAL(triggered()), TWApp::instance(), SLOT(updateScriptsList()));
	connect(showScriptsFolderAction, SIGNAL(triggered()), TWApp::instance(), SLOT(showScriptsFolder()));
	scriptMapper = new QSignalMapper(this);
	connect(scriptMapper, SIGNAL(mapped(QObject*)), this, SLOT(runScript(QObject*)));
	staticScriptMenuItemCount = scriptsMenu->actions().count();
	
	connect(qApp, SIGNAL(scriptListChanged()), this, SLOT(updateScriptsMenu()));
	
	updateScriptsMenu();
}

void
TWScriptable::updateScriptsMenu()
{
	TWScriptManager * scriptManager = TWApp::instance()->getScriptManager();
	
	QList<QAction*> actions = scriptsMenu->actions();
	for (int i = staticScriptMenuItemCount; i < actions.count(); ++i) {
		scriptsMenu->removeAction(actions[i]);
		delete actions[i];
	}
	
	addScriptsToMenu(scriptsMenu, scriptManager->getScripts());
}

int
TWScriptable::addScriptsToMenu(QMenu *menu, TWScriptList *scripts)
{
	int count = 0;
	foreach (QObject *obj, scripts->children()) {
		TWScript *script = qobject_cast<TWScript*>(obj);
		if (script) {
			if (!script->isEnabled())
				continue;
			if (script->getContext().isEmpty() || script->getContext() == metaObject()->className()) {
				printf("Adding script: %s  enabled=%d\n", script->getTitle().toAscii().data(), script->isEnabled());
				QAction *a = menu->addAction(script->getTitle());
				if (!script->getKeySequence().isEmpty())
					a->setShortcut(script->getKeySequence());
//				a->setEnabled(script->isEnabled());
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

	if (!s->isEnabled())
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
	foreach (TWScript *s, TWApp::instance()->getScriptManager()->getHookScripts(hookName)) {
		runScript(s, TWScript::ScriptHook);
	}
}

void
TWScriptable::doAboutScripts()
{
	QString scriptingLink = QString("<a href=\"%1\">%1</a>").arg("http://code.google.com/p/texworks/wiki/ScriptingTeXworks");
	QString aboutText = "<p>";
	aboutText += tr("Scripts may be used to add new commands to %1, "
					"and to extend or modify its behavior.").arg(TEXWORKS_NAME);
	aboutText += "</p><p><small>";
	aboutText += tr("For more information on creating and using scripts, see %1</p>").arg(scriptingLink);
	aboutText += "</small></p><p>";
	aboutText += tr("Scripting languages currently available in this copy of %1:").arg(TEXWORKS_NAME);
	aboutText += "</p><ul>";
	foreach (const TWScriptLanguageInterface* i,
			 TWApp::instance()->getScriptManager()->languages()) {
		aboutText += "<li><a href=\"";
		aboutText += i->scriptLanguageURL();
		aboutText += "\">";
		aboutText += i->scriptLanguageName();
		aboutText += "</a></li>";
	}
	QMessageBox::about(NULL, tr("About Scripts"), aboutText);
}

void
TWScriptable::doManageScripts()
{
	ScriptManager::showManageScripts();
}
