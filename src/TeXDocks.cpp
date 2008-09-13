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
	see <http://tug.org/texworks/>.
*/

#include "TeXDocks.h"

#include "TeXDocument.h"

#include <QTreeWidget>
#include <QHeaderView>
#include <QDomNode>

TeXDock::TeXDock(const QString& title, TeXDocument *doc)
	: QDockWidget(title, doc), document(doc), filled(false)
{
	connect(this, SIGNAL(visibilityChanged(bool)), SLOT(myVisibilityChanged(bool)));
}

TeXDock::~TeXDock()
{
}

void TeXDock::myVisibilityChanged(bool visible)
{
	if (visible && document && !filled) {
		fillInfo();
		filled = true;
	}
}

//////////////// TAGS ////////////////

TagsDock::TagsDock(TeXDocument *doc)
	: TeXDock(tr("Tags"), doc)
{
	tree = new TeXDockTreeWidget(this);
	tree->setAlternatingRowColors(true);
	tree->header()->hide();
	tree->setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
	setWidget(tree);
	connect(doc, SIGNAL(tagListUpdated()), this, SLOT(listChanged()));
}

TagsDock::~TagsDock()
{
}

void TagsDock::fillInfo()
{
	tree->clear();
	const QList<TeXDocument::Tag>& tags = document->getTags();
	if (tags.size() > 0) {
		QTreeWidgetItem *item = 0, *prevZero = 0;
		for (int index = 0; index < tags.size(); ++index) {
			const TeXDocument::Tag& bm = tags[index];
			if (bm.level < 1) {
				prevZero = new QTreeWidgetItem(tree, prevZero, QTreeWidgetItem::UserType);
				prevZero->setText(0, bm.text);
				prevZero->setText(1, QString::number(index));
			}
			else  {
				while (item != 0 && item->type() >= QTreeWidgetItem::UserType + bm.level)
					item = item->parent();
				if (item == 0)
					item = new QTreeWidgetItem(tree, QTreeWidgetItem::UserType + bm.level);
				else
					item = new QTreeWidgetItem(item, QTreeWidgetItem::UserType + bm.level);
				item->setText(0, bm.text);
				item->setText(1, QString::number(index));
				tree->expandItem(item);
			}
		}
		if (prevZero != 0 && item != 0) {
			item = new QTreeWidgetItem(tree, prevZero);
			item->setText(0, QString(0x2014));
			item->setDisabled(true);
		}
		connect(tree, SIGNAL(itemSelectionChanged()), this, SLOT(followTagSelection()));
	} else {
		QTreeWidgetItem *item = new QTreeWidgetItem();
		item->setText(0, tr("No tags"));
		item->setFlags(item->flags() & ~Qt::ItemIsEnabled);
		tree->addTopLevelItem(item);
	}
}

void TagsDock::listChanged()
{
	tree->clear();
	if (document)
		fillInfo();
}

void TagsDock::followTagSelection()
{
	QList<QTreeWidgetItem*> items = tree->selectedItems();
	if (items.count() > 0) {
		QTreeWidgetItem* item = items.first();
		QString dest = item->text(1);
		if (!dest.isEmpty())
			document->goToTag(dest.toInt());
	}
}

TeXDockTreeWidget::TeXDockTreeWidget(QWidget* parent)
	: QTreeWidget(parent)
{
}

TeXDockTreeWidget::~TeXDockTreeWidget()
{
}

QSize TeXDockTreeWidget::sizeHint() const
{
	return QSize(180, 300);
}
