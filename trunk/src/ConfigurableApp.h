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

#ifndef ConfigurableApp_H
#define ConfigurableApp_H

#include <QApplication>
#include <QSettings>

#ifdef Q_WS_MAC
#define QSETTINGS_OBJECT(s) \
			QSettings s(ConfigurableApp::instance()->getSettingsFormat(), QSettings::UserScope, \
						ConfigurableApp::instance()->organizationDomain(), ConfigurableApp::instance()->applicationName())
#else
#define QSETTINGS_OBJECT(s) \
			QSettings s(ConfigurableApp::instance()->getSettingsFormat(), QSettings::UserScope, \
						ConfigurableApp::instance()->organizationName(), ConfigurableApp::instance()->applicationName())
#endif


class ConfigurableApp : public QApplication
{
	Q_OBJECT

public:
	ConfigurableApp(int &argc, char **argv) : QApplication(argc, argv), settingsFormat(QSettings::NativeFormat) { }
	virtual ~ConfigurableApp() { }

	static ConfigurableApp * instance() { return qobject_cast<ConfigurableApp*>(QApplication::instance()); }

	QSettings::Format getSettingsFormat() const { return settingsFormat; }
	void setSettingsFormat(QSettings::Format fmt) { settingsFormat = fmt; }
	
protected:
	QSettings::Format settingsFormat;

};

#endif // !defined(ConfigurableApp)
