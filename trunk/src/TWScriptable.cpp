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
#include "TWScriptAPI.h"
#include "ScriptManager.h"
#include "TWApp.h"

#include <QSignalMapper>
#include <QMenu>
#include <QAction>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QStatusBar>
#include <QToolBar>
#include <QDockWidget>
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

bool JSScript::execute(TWScriptAPI *tw) const
{
	QFile scriptFile(m_Filename);
	if (!scriptFile.open(QIODevice::ReadOnly)) {
		// handle error
		return false;
	}
	QTextStream stream(&scriptFile);
	stream.setCodec(m_Codec);
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
	reloadScripts();
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
		if (qobject_cast<TWScriptLanguageInterface*>(plugin))
			scriptLanguages += plugin;
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
#if QT_VERSION >= 0x040400
		// (At least) Python 2.6 requires the symbols in the secondary libraries
		// to be put in the global scope if modules are imported that load
		// additional shared libraries (e.g. datetime)
		loader.setLoadHints(QLibrary::ExportExternalSymbolsHint);
#endif
		QObject *plugin = loader.instance();
		if (qobject_cast<TWScriptLanguageInterface*>(plugin))
			scriptLanguages += plugin;
	}
}

void TWScriptManager::reloadScripts(bool forceAll /*= false*/)
{
	QSETTINGS_OBJECT(settings);
	QStringList disabled = settings.value("disabledScripts", QStringList()).toStringList();
	QStringList processed;
	
	// canonicalize the paths
	QDir scriptsDir(TWUtils::getLibraryPath("scripts"));
	for (int i = 0; i < disabled.size(); ++i)
		disabled[i] = QFileInfo(scriptsDir.absoluteFilePath(disabled[i])).canonicalFilePath();

	if (forceAll)
		clear();

	reloadScriptsInList(&m_Scripts, processed);
	reloadScriptsInList(&m_Hooks, processed);

	addScriptsInDirectory(scriptsDir, disabled, processed);
	
	ScriptManager::refreshScriptList();
}

void TWScriptManager::reloadScriptsInList(TWScriptList * list, QStringList & processed)
{
	QSETTINGS_OBJECT(settings);
	bool enableScriptsPlugins = settings.value("enableScriptingPlugins", false).toBool();
	
	foreach(QObject * item, list->children()) {
		if (qobject_cast<TWScriptList*>(item))
			reloadScriptsInList(qobject_cast<TWScriptList*>(item), processed);
		else if (qobject_cast<TWScript*>(item)) {
			TWScript * s = qobject_cast<TWScript*>(item);
			if (s->hasChanged()) {
				// File has been removed
				if (!(QFileInfo(s->getFilename()).exists())) {
					delete s;
					continue;
				}
				// File has been changed - reparse; if an error occurs or the
				// script type has changed treat it as if has been removed (and
				// possibly re-add it later)
				TWScript::ScriptType oldType = s->getType();
				if (!s->parseHeader() || s->getType() != oldType) {
					delete s;
					continue;
				}
			}
			if (!enableScriptsPlugins && !qobject_cast<const JSScriptInterface*>(s->getScriptLanguagePlugin())) {
				// the plugin necessary to execute this scripts has been disabled
				delete s;
				continue;
			}
			processed << s->getFilename();
		}
		else {
		} // should never happen
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
	/// \TODO This no longer works since we introduced multiple levels of scripts
/*
	foreach (QObject* obj, scriptList->children()) {
		TWScript *s = qobject_cast<TWScript*>(obj);
		if (!s)
			continue;
		if (*s == *script)
			return false;
	}
*/
	script->setParent(scriptList);
	return true;
}

static bool scriptListLessThan(const TWScriptList* l1, const TWScriptList* l2)
{
	return l1->getName().toLower() < l2->getName().toLower();
}

static bool scriptLessThan(const TWScript* s1, const TWScript* s2)
{
	return s1->getTitle().toLower() < s2->getTitle().toLower();
}


void TWScriptManager::addScriptsInDirectory(TWScriptList *scriptList,
											TWScriptList *hookList,
											const QDir& dir,
											const QStringList& disabled,
											const QStringList& ignore)
{
	QSETTINGS_OBJECT(settings);
	bool scriptingPluginsEnabled = settings.value("enableScriptingPlugins", false).toBool();
	
	foreach (const QFileInfo& info,
			 dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot | QDir::Readable, QDir::DirsLast)) {
		if (info.isDir()) {
			// Only create a new sublist if a matching one doesn't already exist
			TWScriptList *subScriptList = NULL;
			// Note: Using children() returns a const list; findChildren does not
			foreach (TWScriptList * l, scriptList->findChildren<TWScriptList*>()) {
				if (l->getName() == info.fileName()) {
					subScriptList = l;
					break;
				}
			}
			if(!subScriptList) subScriptList = new TWScriptList(scriptList, info.fileName());
			
			// Only create a new sublist if a matching one doesn't already exist
			TWScriptList *subHookList = NULL;
			// Note: Using children() returns a const list; findChildren does not
			foreach (TWScriptList * l, hookList->findChildren<TWScriptList*>()) {
				if (l->getName() == info.fileName()) {
					subHookList = l;
					break;
				}
			}
			if (!subHookList)
				subHookList = new TWScriptList(hookList, info.fileName());
			
			addScriptsInDirectory(subScriptList, subHookList, info.absoluteFilePath(), disabled, ignore);
			if (subScriptList->children().isEmpty())
				delete subScriptList;
			if (subHookList->children().isEmpty())
				delete subHookList;
			continue;
		}
		
		if (ignore.contains(info.absoluteFilePath()))
			continue;

		foreach (QObject * plugin, scriptLanguages) {
			TWScriptLanguageInterface * i = qobject_cast<TWScriptLanguageInterface*>(plugin);
			if (!i)
				continue;
			if (!scriptingPluginsEnabled && !qobject_cast<JSScriptInterface*>(plugin))
				continue;
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
	
	// perform custom sorting
	// since QObject::children() is const, we have to work around that limitation
	// by unsetting all parents first, sort, and finally reset parents in the
	// correct order

	QList<TWScriptList*> childLists; 
	QList<TWScript*> childScripts;

	// Note: we can't use QObject::findChildren here because it's recursive
	const QObjectList& children = scriptList->children();
	foreach (QObject *obj, children) {
		if (TWScript *script = qobject_cast<TWScript*>(obj))
			childScripts.append(script);
		else if (TWScriptList *list = qobject_cast<TWScriptList*>(obj))
			childLists.append(list);
		else { // shouldn't happen
		}
	}
	
	// unset parents; this effectively removes the objects from
	// scriptList->children()
	foreach (TWScript* childScript, childScripts)
		childScript->setParent(NULL);
	foreach (TWScriptList* childList, childLists)
		childList->setParent(NULL);
	
	// sort the sublists
	qSort(childLists.begin(), childLists.end(), scriptListLessThan);
	qSort(childScripts.begin(), childScripts.end(), scriptLessThan);
	
	// add the scripts again, one-by-one
	foreach (TWScript* childScript, childScripts)
		childScript->setParent(scriptList);
	foreach (TWScriptList* childList, childLists)
		childList->setParent(scriptList);
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
	QSETTINGS_OBJECT(settings);
	
	TWScript * s = qobject_cast<TWScript*>(script);
	if (!s || s->getType() != scriptType)
		return;

	if (!settings.value("enableScriptingPlugins", false).toBool() &&
		!qobject_cast<const JSScriptInterface*>(s->getScriptLanguagePlugin())
	) return;

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
	foreach (const QObject * plugin,
			 TWApp::instance()->getScriptManager()->languages()) {
		const TWScriptLanguageInterface * i = qobject_cast<TWScriptLanguageInterface*>(plugin);
		if(!i) continue;
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

void TWScriptable::hideFloatersUnlessThis(QWidget* currWindow)
{
	TWScriptable* p = qobject_cast<TWScriptable*>(currWindow);
	if (p == this)
		return;
	foreach (QObject* child, children()) {
		QToolBar* tb = qobject_cast<QToolBar*>(child);
		if (tb && tb->isVisible() && tb->isFloating()) {
			latentVisibleWidgets.append(tb);
			tb->hide();
			continue;
		}
		QDockWidget* dw = qobject_cast<QDockWidget*>(child);
		if (dw && dw->isVisible() && dw->isFloating()) {
			latentVisibleWidgets.append(dw);
			dw->hide();
			continue;
		}
	}
}

void TWScriptable::showFloaters()
{
	foreach (QWidget* w, latentVisibleWidgets)
	w->show();
	latentVisibleWidgets.clear();
}

void TWScriptable::placeOnLeft()
{
	TWUtils::zoomToHalfScreen(this, false);
}

void TWScriptable::placeOnRight()
{
	TWUtils::zoomToHalfScreen(this, true);
}

void TWScriptable::selectWindow(bool activate)
{
	show();
	raise();
	if (activate)
		activateWindow();
	if (isMinimized())
		showNormal();
}
