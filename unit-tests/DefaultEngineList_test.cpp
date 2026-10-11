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

#include "DefaultEngineList_test.h"
#include "DefaultEngineList.h"

namespace {

const Tw::DefaultEngineList::Definition * findDefinition(const QList<Tw::DefaultEngineList::Definition> & definitions, const QString & name)
{
	for (const Tw::DefaultEngineList::Definition & definition : definitions) {
		if (definition.name == name) {
			return &definition;
		}
	}
	return nullptr;
}

int definitionCount(const QList<Tw::DefaultEngineList::Definition> & definitions, const QString & name)
{
	int count{0};
	for (const Tw::DefaultEngineList::Definition & definition : definitions) {
		if (definition.name == name) {
			++count;
		}
	}
	return count;
}

#if defined(Q_OS_WIN)
const QString contextProgram{QStringLiteral("context.exe")};
const QString texexecProgram{QStringLiteral("texexec.exe")};
#else
const QString contextProgram{QStringLiteral("context")};
const QString texexecProgram{QStringLiteral("texexec")};
#endif

} // namespace

namespace UnitTest {

void TestDefaultEngineList::contextLuaMetaTeX()
{
	const QList<Tw::DefaultEngineList::Definition> definitions{Tw::DefaultEngineList::definitions()};
	const QString name{QStringLiteral("ConTeXt (LuaMetaTeX)")};
	QCOMPARE(definitionCount(definitions, name), 1);
	const Tw::DefaultEngineList::Definition * definition{findDefinition(definitions, name)};
	QVERIFY(definition != nullptr);
	QCOMPARE(definition->program, contextProgram);
	QCOMPARE(definition->arguments, QStringList({QStringLiteral("--synctex=repeat"), QStringLiteral("$fullname")}));
	QVERIFY(definition->arguments.contains(QStringLiteral("--luatex")) == false);
	QVERIFY(definition->showPdf);
	QCOMPARE(definition->sourceLanguage, QStringLiteral("context"));
}

void TestDefaultEngineList::contextLuaTeX()
{
	const QList<Tw::DefaultEngineList::Definition> definitions{Tw::DefaultEngineList::definitions()};
	const QString name{QStringLiteral("ConTeXt (LuaTeX)")};
	QCOMPARE(definitionCount(definitions, name), 1);
	const Tw::DefaultEngineList::Definition * definition{findDefinition(definitions, name)};
	QVERIFY(definition != nullptr);
	QCOMPARE(definition->program, contextProgram);
	QCOMPARE(definition->arguments, QStringList({QStringLiteral("--synctex=repeat"), QStringLiteral("--luatex"), QStringLiteral("$fullname")}));
	QVERIFY(definition->showPdf);
	QCOMPARE(definition->sourceLanguage, QStringLiteral("context"));
}

void TestDefaultEngineList::contextDefinitionsAreDistinct()
{
	const QList<Tw::DefaultEngineList::Definition> definitions{Tw::DefaultEngineList::definitions()};
	const Tw::DefaultEngineList::Definition * luaMetaTeX{findDefinition(definitions, QStringLiteral("ConTeXt (LuaMetaTeX)"))};
	const Tw::DefaultEngineList::Definition * luaTeX{findDefinition(definitions, QStringLiteral("ConTeXt (LuaTeX)"))};
	QVERIFY(luaMetaTeX != nullptr);
	QVERIFY(luaTeX != nullptr);
	QVERIFY(luaMetaTeX->name != luaTeX->name);
	QVERIFY(luaMetaTeX->arguments != luaTeX->arguments);
}

void TestDefaultEngineList::historicalContextEngines()
{
	const QList<Tw::DefaultEngineList::Definition> definitions{Tw::DefaultEngineList::definitions()};
	const Tw::DefaultEngineList::Definition * pdfTeX{findDefinition(definitions, QStringLiteral("ConTeXt (pdfTeX)"))};
	const Tw::DefaultEngineList::Definition * xeTeX{findDefinition(definitions, QStringLiteral("ConTeXt (XeTeX)"))};
	QVERIFY(pdfTeX != nullptr);
	QVERIFY(xeTeX != nullptr);
	QCOMPARE(pdfTeX->program, texexecProgram);
	QCOMPARE(pdfTeX->arguments, QStringList({QStringLiteral("--synctex"), QStringLiteral("$fullname")}));
	QVERIFY(pdfTeX->showPdf);
	QCOMPARE(pdfTeX->sourceLanguage, QStringLiteral("context"));
	QCOMPARE(xeTeX->program, texexecProgram);
	QCOMPARE(xeTeX->arguments, QStringList({QStringLiteral("--synctex"), QStringLiteral("--xtx"), QStringLiteral("$fullname")}));
	QVERIFY(xeTeX->showPdf);
	QCOMPARE(xeTeX->sourceLanguage, QStringLiteral("context"));

	const Tw::DefaultEngineList::Definition * pdfLaTeX{findDefinition(definitions, QStringLiteral("pdfLaTeX"))};
	QVERIFY(pdfLaTeX != nullptr);
	QVERIFY(pdfLaTeX->sourceLanguage.isEmpty());
}

} // namespace UnitTest

QTEST_APPLESS_MAIN(UnitTest::TestDefaultEngineList)
