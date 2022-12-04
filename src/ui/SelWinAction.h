/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2022  Jonathan Kew, Stefan Löffler

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

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/

#ifndef UI_SelWinAction_H
#define UI_SelWinAction_H

#include <QAction>

namespace Tw {

namespace UI {

// this special QAction class is used in Window menus, so that it's easy to recognize the dynamically-created items
class SelWinAction : public QAction
{
	Q_OBJECT

public:
	SelWinAction(QObject *parent, const QString & fileName, const QString &label);
};

} // namespace UI

} // namespace Tw

#endif // !defined(UI_SelWinAction_H)
