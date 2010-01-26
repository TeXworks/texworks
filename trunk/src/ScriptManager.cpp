/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2010  Jonathan Kew & Stefan Löffler

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

#include "ScriptManager.h"
#include "TWApp.h"
#include "TWScript.h"

#include <QLabel>
#include <QCloseEvent>

ScriptManager * ScriptManager::gManageScriptsWindow = NULL;
QRect           ScriptManager::gGeometry;

void ScriptManager::init()
{
	setupUi(this);
	
	hookTree->header()->hide();
	standaloneTree->header()->hide();
	
	populateTree();
	
	connect(scriptTabs, SIGNAL(currentChanged(int)), this, SLOT(treeSelectionChanged()));

	connect(hookTree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemClicked(QTreeWidgetItem *, int)));
	connect(hookTree, SIGNAL(itemSelectionChanged()), this, SLOT(treeSelectionChanged()));
	connect(standaloneTree, SIGNAL(itemClicked(QTreeWidgetItem *, int)), this, SLOT(treeItemClicked(QTreeWidgetItem *, int)));
	connect(standaloneTree, SIGNAL(itemSelectionChanged()), this, SLOT(treeSelectionChanged()));

	connect(this, SIGNAL(scriptListChanged()), qApp, SIGNAL(scriptListChanged()));
}

void ScriptManager::closeEvent(QCloseEvent * event)
{
	gGeometry = geometry();
	hide();
	event->accept();
}

/*static*/
void ScriptManager::showManageScripts()
{
	if (!gManageScriptsWindow)
		gManageScriptsWindow = new ScriptManager(NULL);

	if (!gGeometry.isNull())
		gManageScriptsWindow->setGeometry(gGeometry);
	
	gManageScriptsWindow->show();
	gManageScriptsWindow->raise();
	gManageScriptsWindow->activateWindow();
}

void ScriptManager::populateTree()
{
	TWScriptManager * scriptManager = TWApp::instance()->getScriptManager();
	TWScriptList * scripts = scriptManager->getScripts();
	TWScriptList * hooks = scriptManager->getHookScripts();
	
	hookTree->clear();
	standaloneTree->clear();
	
	populateTree(hookTree, NULL, hooks);
	populateTree(standaloneTree, NULL, scripts);
	
	hookTree->expandAll();
	standaloneTree->expandAll();
}

void ScriptManager::populateTree(QTreeWidget * tree, QTreeWidgetItem * parentItem, const TWScriptList * scripts)
{
	QTreeWidgetItem * item;

	foreach (QObject * obj, scripts->children()) {
		TWScript * script = qobject_cast<TWScript*>(obj);
		if (script && script->getType() != TWScript::ScriptUnknown) {
			QStringList strList(script->getTitle());
			item = parentItem ? new QTreeWidgetItem(parentItem, strList) : new QTreeWidgetItem(tree, strList);
			item->setData(0, Qt::UserRole, qVariantFromValue((void*)script));
			item->setCheckState(0, script->isEnabled() ? Qt::Checked : Qt::Unchecked);
			continue;
		}
		TWScriptList * list = qobject_cast<TWScriptList*>(obj);
		if (list) {
			QStringList strList(list->getName());
			item = parentItem ? new QTreeWidgetItem(parentItem, strList) : new QTreeWidgetItem(tree, strList);
			QFont f = item->font(0);
			f.setBold(true);
			item->setFont(0, f);
			populateTree(NULL, item, list);
		}
	}
}

void ScriptManager::treeItemClicked(QTreeWidgetItem * item, int /*column*/)
{
	TWScript * s = static_cast<TWScript*>(item->data(0, Qt::UserRole).value<void*>());
	if (s) {
		s->setEnabled(item->checkState(0) == Qt::Checked);
		emit scriptListChanged();
	}
}

void ScriptManager::treeSelectionChanged()
{
	details->setPlainText("");

	QTreeWidget * tree = scriptTabs->currentWidget() == standaloneTab ? standaloneTree : hookTree;
	QList<QTreeWidgetItem*> selection = tree->selectedItems();
	if (selection.size() != 1)
		return;
	
	TWScript * s = static_cast<TWScript*>(selection[0]->data(0, Qt::UserRole).value<void*>());
	if (!s)
		return;

	QString rows;
	addDetailsRow(rows, tr("Name: "), s->getTitle());
	addDetailsRow(rows, tr("Context: "), s->getContext());
	addDetailsRow(rows, tr("Description: "), s->getDescription());
	addDetailsRow(rows, tr("Author: "), s->getAuthor());
	addDetailsRow(rows, tr("Version: "), s->getVersion());
	addDetailsRow(rows, tr("Shortcut: "), s->getKeySequence().toString());
	addDetailsRow(rows, tr("File: "), QFileInfo(s->getFilename()).fileName());
	
	const TWScriptLanguageInterface * sli = s->getScriptLanguageInterface();
	QString url = sli->scriptLanguageURL();
	QString str = sli->scriptLanguageName();
	if (!url.isEmpty())
		str = "<a href=\"" + url + "\">" + str + "</a>";
	addDetailsRow(rows, tr("Language: "), str);

	if (s->getType() == TWScript::ScriptHook)
		addDetailsRow(rows, tr("Hook: "), s->getHook());

	details->setHtml("<table>" + rows + "</table>");
}

void ScriptManager::addDetailsRow(QString& html, const QString label, const QString value)
{
	if (!value.isEmpty())
		html += "<tr><td>" + label + "</td><td>" + value + "</td></tr>";
}
