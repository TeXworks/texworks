/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2022  Stefan Löffler

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
#include "LuaScripting_test.h"
#include "MockScriptingAPI.h"

using namespace Tw::Scripting;

#if STATIC_LUA_SCRIPTING_PLUGIN
#include <QtPlugin>
Q_IMPORT_PLUGIN(LuaScriptInterface)
#endif

Q_DECLARE_METATYPE(QSharedPointer<Script>)
Q_DECLARE_METATYPE(Script::ScriptType)

namespace UnitTest {

void TestLuaScripting::initTestCase()
{
	luaSI = nullptr;
	foreach (QObject *plugin, QPluginLoader::staticInstances()) {
		Tw::Scripting::ScriptLanguageInterface * si = qobject_cast<Tw::Scripting::ScriptLanguageInterface*>(plugin);
		if (si && si->scriptLanguageName() == QStringLiteral("Lua")) {
			luaSI = si;
			break;
		}
	}

	// Look for scripting plugins alongside the executable
	QDir pluginsDir(QCoreApplication::applicationDirPath());
	foreach (QString fileName, pluginsDir.entryList(QDir::Files)) {
		loader.setFileName(pluginsDir.absoluteFilePath(fileName));
		// (At least) Python 2.6 requires the symbols in the secondary libraries
		// to be put in the global scope if modules are imported that load
		// additional shared libraries (e.g. datetime)
		loader.setLoadHints(QLibrary::ExportExternalSymbolsHint);
		QObject *plugin = loader.instance();
		Tw::Scripting::ScriptLanguageInterface * si = qobject_cast<Tw::Scripting::ScriptLanguageInterface*>(plugin);
		if (si && si->scriptLanguageName() == QStringLiteral("Lua")) {
			luaSI = si;
			break;
		}
		else {
			loader.unload();
			loader.setFileName(QString());
		}
	}

	if (!luaSI)
		QFAIL("Could not find LuaScriptInterface");
}

void TestLuaScripting::cleanupTestCase()
{
	loader.unload();
}

void TestLuaScripting::scriptLanguageName()
{
	QCOMPARE(luaSI->scriptLanguageName(), QStringLiteral("Lua"));
}

void TestLuaScripting::scriptLanguageURL()
{
	QCOMPARE(luaSI->scriptLanguageURL(), QStringLiteral("http://www.lua.org/"));
}

void TestLuaScripting::canHandleFile()
{
	QFileInfo fiNull;
	QFileInfo fiJs(QStringLiteral("file.js"));
	QFileInfo fiPy(QStringLiteral("file.py"));
	QFileInfo fiLua(QStringLiteral("file.lua"));
	QFileInfo fiTex(QStringLiteral("file.tex"));

	QVERIFY(luaSI->canHandleFile(fiNull) == false);
	QVERIFY(luaSI->canHandleFile(fiJs) == false);
	QVERIFY(luaSI->canHandleFile(fiPy) == false);
	QVERIFY(luaSI->canHandleFile(fiLua));
	QVERIFY(luaSI->canHandleFile(fiTex) == false);
}

void TestLuaScripting::execute()
{
	QSharedPointer<Script> s1 = QSharedPointer<Script>(luaSI->newScript(QStringLiteral("does-not-exist")));
	QSharedPointer<Script> s2 = QSharedPointer<Script>(luaSI->newScript(QStringLiteral("script1.lua")));
//	QSharedPointer<Script> s3 = QSharedPointer<Script>(luaSI->newScript(QStringLiteral("script2.js")));

	{
		MockTarget target;
		MockAPI api(s1.data(), &target);
		QVERIFY(s1->run(api) == false);
	}
	{
		MockTarget target;
		MockAPI api(s2.data(), &target);

		s2->setGlobal("TwNil", QVariant());
		s2->setGlobal("TwBool", QVariant::fromValue(true));
		s2->setGlobal("TwDouble", QVariant::fromValue(4.2));
		s2->setGlobal("TwString", QVariant::fromValue(QStringLiteral("Ok")));
		s2->setGlobal("TwList", QVariantList{QStringLiteral("Fourty"), 2.});
		s2->setGlobal("TwMap", QVariantMap{{QStringLiteral("k"), QStringLiteral("v")}});
		s2->setGlobal("TwHash", QVariantHash{{QStringLiteral("k"), QStringLiteral("v")}});

		if (!s2->run(api)) {
			qDebug() << api.GetResult().toString();
			QFAIL("An error occurred during Lua execution");
		}

		QCOMPARE(qobject_cast<MockTarget*>(api.GetTarget())->text, QStringLiteral("It works!"));
		QCOMPARE(api.GetResult(), QVariant(QVariantList({1., 2., 3.})));
		QVERIFY(s2->hasGlobal(QStringLiteral("LuaNil")));
		QCOMPARE(s2->getGlobal(QStringLiteral("LuaNil")), QVariant());
		QVERIFY(s2->hasGlobal(QStringLiteral("LuaBool")));
		QCOMPARE(s2->getGlobal(QStringLiteral("LuaBool")), QVariant(true));
		QVERIFY(s2->hasGlobal(QStringLiteral("LuaArray")));
		QCOMPARE(s2->getGlobal(QStringLiteral("LuaArray")), QVariant(QVariantList{QStringLiteral("Hello"), 42., false}));
		QVERIFY(s2->hasGlobal(QStringLiteral("LuaMap")));
		QCOMPARE(s2->getGlobal(QStringLiteral("LuaMap")), QVariant(QVariantMap{
			{QStringLiteral("key"), QStringLiteral("value")},
			{QStringLiteral("1"), 0.}
		}));
		QVERIFY(s2->hasGlobal(QStringLiteral("LuaQObject*")));
		QCOMPARE(s2->getGlobal(QStringLiteral("LuaQObject*")), QVariant::fromValue(&api));
	}


	/*	{
		MockAPI api(js3.data());
		QVERIFY(js3->run(api) == false);
		QCOMPARE(api.GetResult(), QVariant(QStringLiteral("an exception")));
	}
	*/
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestLuaScripting)
