/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2011  Jonathan Kew, Stefan Löffler

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
	see <http://www.tug.org/texworks/>.
*/

#ifndef ClickableLabel_H
#define ClickableLabel_H

#include <QLabel>
#include <QMouseEvent>
#include <QApplication>

class ClickableLabel : public QLabel
{
	Q_OBJECT
public:
	ClickableLabel(QWidget * parent = 0, Qt::WindowFlags f = 0) : QLabel(parent, f) { }
	ClickableLabel(const QString & text, QWidget * parent = 0, Qt::WindowFlags f = 0) : QLabel(text, parent, f) { }
	virtual ~ClickableLabel() { }
	
signals:
	void mouseDoubleClick(QMouseEvent * event);
	void mouseLeftClick(QMouseEvent * event);
	void mouseMiddleClick(QMouseEvent * event);
	void mouseRightClick(QMouseEvent * event);

protected:
	virtual void mouseDoubleClickEvent(QMouseEvent * event) { emit mouseDoubleClick(event); }
	virtual void mousePressEvent(QMouseEvent * event) { mouseStartPoint = event->pos(); }
	virtual void mouseReleaseEvent(QMouseEvent * event) {
		if ((event->pos() - mouseStartPoint).manhattanLength() < QApplication::startDragDistance()) {
			switch (event->button()) {
				case Qt::LeftButton:
					emit mouseLeftClick(event);
					break;
				case Qt::RightButton:
					emit mouseRightClick(event);
					break;
				case Qt::MidButton:
					emit mouseMiddleClick(event);
					break;
				default:
					break;
			}
		}
	}

	QPoint mouseStartPoint;
};

#endif // !defined(ClickableLabel_H)
