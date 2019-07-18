/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2009-2019  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

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

#ifndef JSScriptInterface_H
#define JSScriptInterface_H

#include "TWScript.h"

// for JSScript, we provide a plugin-like factory, but it's actually compiled
// and linked directly with the main application (at least for now)
class JSScriptInterface : public QObject, public TWScriptLanguageInterface
{
	Q_OBJECT
	Q_INTERFACES(TWScriptLanguageInterface)

public:
	JSScriptInterface() {}
	virtual ~JSScriptInterface() {}

	virtual TWScript* newScript(const QString& fileName);

	virtual QString scriptLanguageName() const { return QString::fromLatin1("QtScript"); }
	virtual QString scriptLanguageURL() const { return QString::fromLatin1("http://doc.qt.io/qt-5/qtscript-index.html"); }
	virtual bool canHandleFile(const QFileInfo& fileInfo) const { return fileInfo.suffix() == QLatin1String("js"); }
};

#endif // !defined(JSScriptInterface_H)
