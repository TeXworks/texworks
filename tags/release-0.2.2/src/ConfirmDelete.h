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

#ifndef ConfirmDelete_H
#define ConfirmDelete_H

#include <QDialog>
#include <QStringList>

#include "TWUtils.h"

#include "ui_ConfirmDelete.h"

class ConfirmDelete : public QDialog, private Ui::ConfirmDelete
{
	Q_OBJECT

public:
	ConfirmDelete(QWidget *parent = NULL);
	virtual ~ConfirmDelete();

	static void doConfirmDelete(const QDir& dir, const QStringList& fileList);

private slots:

private:
	void init();
};

#endif
