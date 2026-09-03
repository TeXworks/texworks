/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <https://tug.org/texworks/>.
*/

#include "DefaultEngineList.h"

#if defined(Q_OS_WIN)
#define ENGINE_EXE ".exe"
#else
#define ENGINE_EXE
#endif

namespace Tw {
namespace DefaultEngineList {

QList<Definition> definitions()
{
	//	<< Engine("LaTeXmk", "latexmk" EXE, QStringList("-e") <<
	//			  "$pdflatex=q/pdflatex -synctex=1 %O %S/" << "-pdf" << "$fullname", true)
	return QList<Definition>()
	    << Definition{QString::fromLatin1("pdfTeX"), QString::fromLatin1("pdftex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("pdfLaTeX"), QString::fromLatin1("pdflatex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("LuaTeX"), QString::fromLatin1("luatex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("LuaLaTeX"), QString::fromLatin1("lualatex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("XeTeX"), QString::fromLatin1("xetex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("XeLaTeX"), QString::fromLatin1("xelatex" ENGINE_EXE), QStringList(QString::fromLatin1("$synctexoption")) << QString::fromLatin1("$fullname"), true, QString{}}
	    << Definition{QString::fromLatin1("ConTeXt (LuaMetaTeX)"), QString::fromLatin1("context" ENGINE_EXE), QStringList(QString::fromLatin1("--synctex=repeat")) << QString::fromLatin1("$fullname"), true, QString::fromLatin1("context")}
	    << Definition{QString::fromLatin1("ConTeXt (LuaTeX)"), QString::fromLatin1("context" ENGINE_EXE), QStringList(QString::fromLatin1("--synctex=repeat")) << QString::fromLatin1("--luatex") << QString::fromLatin1("$fullname"), true, QString::fromLatin1("context")}
	    << Definition{QString::fromLatin1("ConTeXt (pdfTeX)"), QString::fromLatin1("texexec" ENGINE_EXE), QStringList(QString::fromLatin1("--synctex")) << QString::fromLatin1("$fullname"), true, QString::fromLatin1("context")}
	    << Definition{QString::fromLatin1("ConTeXt (XeTeX)"), QString::fromLatin1("texexec" ENGINE_EXE), QStringList(QString::fromLatin1("--synctex")) << QString::fromLatin1("--xtx") << QString::fromLatin1("$fullname"), true, QString::fromLatin1("context")}
	    << Definition{QString::fromLatin1("BibTeX"), QString::fromLatin1("bibtex" ENGINE_EXE), QStringList(QString::fromLatin1("$basename")), false, QString{}}
	    << Definition{QString::fromLatin1("Biber"), QString::fromLatin1("biber" ENGINE_EXE), QStringList(QString::fromLatin1("$basename")), false, QString{}}
	    << Definition{QString::fromLatin1("MakeIndex"), QString::fromLatin1("makeindex" ENGINE_EXE), QStringList(QString::fromLatin1("$basename")), false, QString{}};
}

} // namespace DefaultEngineList
} // namespace Tw
