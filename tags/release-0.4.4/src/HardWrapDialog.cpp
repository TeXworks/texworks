/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-2012  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

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

#include "HardWrapDialog.h"
#include "TWApp.h"

HardWrapDialog::HardWrapDialog(QWidget *parent)
	: QDialog(parent)
{
	setupUi(this);
	init();
}

void
HardWrapDialog::init()
{
	QSETTINGS_OBJECT(settings);
	int	wrapWidth = settings.value("hardWrapWidth", kDefault_HardWrapWidth).toInt();
	spinbox_charCount->setValue(wrapWidth);
	spinbox_charCount->selectAll();

	bool wrapToWindow = settings.value("hardWrapToWindow", false).toBool();
	radio_currentWidth->setChecked(wrapToWindow);
	radio_fixedLineLength->setChecked(!wrapToWindow);
	
	bool rewrapParagraphs = settings.value("hardWrapRewrap", false).toBool();
	checkbox_rewrap->setChecked(rewrapParagraphs);

#ifdef Q_WS_MAC
	setWindowFlags(Qt::Sheet);
#endif
}

void
HardWrapDialog::saveSettings()
{
	QSETTINGS_OBJECT(settings);
	settings.setValue("hardWrapWidth", spinbox_charCount->value());
	settings.setValue("hardWrapToWindow", radio_currentWidth->isChecked());
	settings.setValue("hardWrapRewrap", checkbox_rewrap->isChecked());
}
