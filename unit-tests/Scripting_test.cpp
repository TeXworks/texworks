/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2024  Stefan Löffler

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

#include "Scripting_test.h"

#include "MockScriptingAPI.h"
#include "scripting/ECMAScriptInterface.h"
#if WITH_QTSCRIPT
#	include "scripting/JSScriptInterface.h"
#endif

#include <memory>

#if WITH_QTSCRIPT
// JSScript has settings.value(QString::fromLatin1("scriptDebugger"), false)
// so we mock the relevant Settings method here to not have to pull in the
// entire chain of Settings' dependencies
#include "Settings.h"
namespace Tw {
QVariant Settings::value(KeyType key, const QVariant & defaultValue) const
{
	Q_UNUSED(key)
	return defaultValue;
}
} // namespace Tw
#endif

using namespace Tw::Scripting;

Q_DECLARE_METATYPE(QSharedPointer<Script>)
Q_DECLARE_METATYPE(Script::ScriptType)

namespace UnitTest {

void TestScripting::scriptLanguageName()
{
#if WITH_QTSCRIPT
	Tw::Scripting::JSScriptInterface js(this);
	QCOMPARE(js.scriptLanguageName(), QStringLiteral("QtScript"));
#endif

	Tw::Scripting::ECMAScriptInterface es(this);
	QCOMPARE(es.scriptLanguageName(), QStringLiteral("ECMAScript"));
}

void TestScripting::scriptLanguageURL()
{
#if WITH_QTSCRIPT
	Tw::Scripting::JSScriptInterface js(this);
	QCOMPARE(js.scriptLanguageURL(), QStringLiteral("http://doc.qt.io/qt-5/qtscript-index.html"));
#endif

	Tw::Scripting::ECMAScriptInterface es(this);
	QCOMPARE(es.scriptLanguageURL(), QStringLiteral("https://doc.qt.io/qt-5/qjsengine.html"));
}

void TestScripting::canHandleFile()
{
	QFileInfo fiNull;
	QFileInfo fiEs(QStringLiteral("file.es"));
	QFileInfo fiJs(QStringLiteral("file.js"));
	QFileInfo fiPy(QStringLiteral("file.py"));
	QFileInfo fiLua(QStringLiteral("file.lua"));
	QFileInfo fiTex(QStringLiteral("file.tex"));

#if WITH_QTSCRIPT
	Tw::Scripting::JSScriptInterface js(this);
	QVERIFY(js.canHandleFile(fiNull) == false);
	QVERIFY(js.canHandleFile(fiEs) == false);
	QVERIFY(js.canHandleFile(fiJs));
	QVERIFY(js.canHandleFile(fiPy) == false);
	QVERIFY(js.canHandleFile(fiLua) == false);
	QVERIFY(js.canHandleFile(fiTex) == false);
#endif

	Tw::Scripting::ECMAScriptInterface es(this);
	QVERIFY(es.canHandleFile(fiNull) == false);
	QVERIFY(es.canHandleFile(fiEs));
	QVERIFY(es.canHandleFile(fiJs));
	QVERIFY(es.canHandleFile(fiPy) == false);
	QVERIFY(es.canHandleFile(fiLua) == false);
	QVERIFY(es.canHandleFile(fiTex) == false);
}

void TestScripting::isEnabled()
{
#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);
	std::unique_ptr<Script> js{jsi.newScript(QString())};

	QVERIFY(js->isEnabled());
	js->setEnabled(false);
	QVERIFY(js->isEnabled() == false);
	js->setEnabled(true);
	QVERIFY(js->isEnabled());
#endif

	ECMAScriptInterface esi(this);
	std::unique_ptr<Script> es{esi.newScript(QString())};

	QVERIFY(es->isEnabled());
	es->setEnabled(false);
	QVERIFY(es->isEnabled() == false);
	es->setEnabled(true);
	QVERIFY(es->isEnabled());
}

void TestScripting::getScriptLanguagePlugin()
{
#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);
	QCOMPARE(std::unique_ptr<Script>(jsi.newScript(QString()))->getScriptLanguagePlugin(), &jsi);
#endif

	ECMAScriptInterface esi(this);
	QCOMPARE(std::unique_ptr<Script>(esi.newScript(QString()))->getScriptLanguagePlugin(), &esi);
}

void TestScripting::getFilename()
{
	QString invalid = QStringLiteral("does-not-exist");
#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);

	QCOMPARE(std::unique_ptr<Script>(jsi.newScript(QString()))->getFilename(), QString());
	QCOMPARE(std::unique_ptr<Script>(jsi.newScript(invalid))->getFilename(), invalid);
#endif

	ECMAScriptInterface esi(this);

	QCOMPARE(std::unique_ptr<Script>(esi.newScript(QString()))->getFilename(), QString());
	QCOMPARE(std::unique_ptr<Script>(esi.newScript(invalid))->getFilename(), invalid);
}

void TestScripting::globals()
{
	QString key = QStringLiteral("myGlobal");
	QVariant val(42.f);

#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);
	QSharedPointer<Script> js = QSharedPointer<Script>(jsi.newScript(QString()));

	QVERIFY(js->hasGlobal(key) == false);
	QCOMPARE(js->getGlobal(key), QVariant());
	QVERIFY(js->hasGlobal(key) == false);
	js->unsetGlobal(key);
	QVERIFY(js->hasGlobal(key) == false);
	js->setGlobal(key, val);
	QVERIFY(js->hasGlobal(key));
	QCOMPARE(js->getGlobal(key), val);
	js->unsetGlobal(key);
	QVERIFY(js->hasGlobal(key) == false);
#endif

	ECMAScriptInterface esi(this);
	QSharedPointer<Script> es = QSharedPointer<Script>(esi.newScript(QString()));

	QVERIFY(es->hasGlobal(key) == false);
	QCOMPARE(es->getGlobal(key), QVariant());
	QVERIFY(es->hasGlobal(key) == false);
	es->unsetGlobal(key);
	QVERIFY(es->hasGlobal(key) == false);
	es->setGlobal(key, val);
	QVERIFY(es->hasGlobal(key));
	QCOMPARE(es->getGlobal(key), val);
	es->unsetGlobal(key);
	QVERIFY(es->hasGlobal(key) == false);
}

void TestScripting::parseHeader_data()
{
	QTest::addColumn<QSharedPointer<Script>>("script");
	QTest::addColumn<bool>("canParse");
	QTest::addColumn<Script::ScriptType>("type");
	QTest::addColumn<QString>("title");
	QTest::addColumn<QString>("description");
	QTest::addColumn<QString>("author");
	QTest::addColumn<QString>("version");
	QTest::addColumn<QString>("hook");
	QTest::addColumn<QString>("context");
	QTest::addColumn<QKeySequence>("keySequence");

#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);

	QTest::newRow("invalid (JS)") << QSharedPointer<Script>(jsi.newScript(QString()))
							 << false
							 << Script::ScriptType::ScriptUnknown
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QKeySequence();
	QTest::newRow("script1.js (JS)") << QSharedPointer<Script>(jsi.newScript(QStringLiteral("script1.js")))
								<< true
								<< Script::ScriptType::ScriptStandalone
								<< QStringLiteral("Tw.insertText test")
								<< QStringLiteral(u"This is a unicode string 🤩")
								<< QStringLiteral(u"Stefan Löffler")
								<< QStringLiteral("0.0.1")
								<< QString()
								<< QStringLiteral("TeXDocument")
								<< QKeySequence(QStringLiteral("Ctrl+Alt+Shift+I"));
	QTest::newRow("script2.js (JS)") << QSharedPointer<Script>(jsi.newScript(QStringLiteral("script2.js")))
								<< false
								<< Script::ScriptType::ScriptUnknown
								<< QStringLiteral("Exception test")
								<< QString()
								<< QStringLiteral(u"Stefan Löffler")
								<< QStringLiteral("0.0.1")
								<< QString()
								<< QString()
								<< QKeySequence();
#endif

	ECMAScriptInterface esi(this);

	QTest::newRow("invalid (ECMA)") << QSharedPointer<Script>(esi.newScript(QString()))
							 << false
							 << Script::ScriptType::ScriptUnknown
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QKeySequence();
	QTest::newRow("script1.js (ECMA)") << QSharedPointer<Script>(esi.newScript(QStringLiteral("script1.js")))
								<< true
								<< Script::ScriptType::ScriptStandalone
								<< QStringLiteral("Tw.insertText test")
								<< QStringLiteral(u"This is a unicode string 🤩")
								<< QStringLiteral(u"Stefan Löffler")
								<< QStringLiteral("0.0.1")
								<< QString()
								<< QStringLiteral("TeXDocument")
								<< QKeySequence(QStringLiteral("Ctrl+Alt+Shift+I"));
	QTest::newRow("script2.js (ECMA)") << QSharedPointer<Script>(esi.newScript(QStringLiteral("script2.js")))
								<< false
								<< Script::ScriptType::ScriptUnknown
								<< QStringLiteral("Exception test")
								<< QString()
								<< QStringLiteral(u"Stefan Löffler")
								<< QStringLiteral("0.0.1")
								<< QString()
								<< QString()
								<< QKeySequence();
}

void TestScripting::parseHeader()
{
	QFETCH(QSharedPointer<Script>, script);
	QFETCH(bool, canParse);
	QFETCH(Script::ScriptType, type);
	QFETCH(QString, title);
	QFETCH(QString, description);
	QFETCH(QString, author);
	QFETCH(QString, version);
	QFETCH(QString, hook);
	QFETCH(QString, context);
	QFETCH(QKeySequence, keySequence);

	QCOMPARE(script->parseHeader(), canParse);

	QCOMPARE(script->getType(), type);
	QCOMPARE(script->getTitle(), title);
	QCOMPARE(script->getDescription(), description);
	QCOMPARE(script->getAuthor(), author);
	QCOMPARE(script->getVersion(), version);
	QCOMPARE(script->getKeySequence(), keySequence);
	QCOMPARE(script->getHook(), hook);
	QCOMPARE(script->getContext(), context);
}

void TestScripting::mocks()
{
	ScriptObject so{std::unique_ptr<Script>(ECMAScriptInterface(this).newScript(QString()))};
	MockTarget target;
	MockAPI api(&so, &target);

	QCOMPARE(api.self(), &api);
	QCOMPARE(api.GetApp(), &api);
	QCOMPARE(api.GetScript(), &so);

	QCOMPARE(api.GetResult(), QVariant());
	{
		QVariant testVariant(42.3);
		api.SetResult(testVariant);
		QCOMPARE(api.GetResult(), testVariant);
	}

	QCOMPARE(api.strlen(QStringLiteral("Test")), 4);
	QCOMPARE(api.platform(), QString());
	QCOMPARE(api.getQtVersion(), QT_VERSION);

	{
		QMap<QString, QVariant> retVal = {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), 0},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
			{QStringLiteral("output"), QString()}
		};
		QCOMPARE(api.system(QString()), retVal);
	}
	{
		QMap<QString, QVariant> retVal = {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
		QCOMPARE(api.launchFile(QString()), retVal);
	}
	QCOMPARE(api.writeFile(QString(), QString()), static_cast<int>(ScriptAPIInterface::SystemAccess_Failed));
	{
		QMap<QString, QVariant> retVal = {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), QString()},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
		QCOMPARE(api.readFile(QString()), retVal);
	}
	QCOMPARE(api.fileExists(QString()), static_cast<int>(ScriptAPIInterface::SystemAccess_Failed));
	QCOMPARE(api.information(nullptr, QString(), QString()), static_cast<int>(QMessageBox::NoButton));
	QCOMPARE(api.question(nullptr, QString(), QString()), static_cast<int>(QMessageBox::NoButton));
	QCOMPARE(api.warning(nullptr, QString(), QString()), static_cast<int>(QMessageBox::NoButton));
	QCOMPARE(api.critical(nullptr, QString(), QString()), static_cast<int>(QMessageBox::NoButton));
	QCOMPARE(api.getInt(nullptr, QString(), QString(), 42), QVariant(42));
	QCOMPARE(api.getDouble(nullptr, QString(), QString(), 42.3), QVariant(42.3));
	QCOMPARE(api.getItem(nullptr, QString(), QString(), {QStringLiteral("1"), QStringLiteral("2")}, 1), QVariant(QStringLiteral("2")));
	QCOMPARE(api.getText(nullptr, QString(), QString(), QStringLiteral("42.3")), QVariant(QStringLiteral("42.3")));
	api.yield();
	QCOMPARE(api.progressDialog(nullptr), static_cast<QWidget*>(nullptr));
	QCOMPARE(api.createUIFromString(QString()), static_cast<QWidget*>(nullptr));
	QCOMPARE(api.createUI(QString()), static_cast<QWidget*>(nullptr));
	QCOMPARE(api.findChildWidget(nullptr, QString()), static_cast<QWidget*>(nullptr));
	QVERIFY(api.makeConnection(nullptr, QString(), nullptr, QString()) == false);
	QMap<QString, QVariant> emptyDictList;
	QCOMPARE(api.getDictionaryList(), emptyDictList);
	QCOMPARE(api.getEngineList(), {});
	QVERIFY(api.mayExecuteSystemCommand(QString(), nullptr) == false);
	QVERIFY(api.mayWriteFile(QString(), nullptr) == false);
	QVERIFY(api.mayReadFile(QString(), nullptr) == false);
}

void TestScripting::execute()
{
#if WITH_QTSCRIPT
	JSScriptInterface jsi(this);

	ScriptObject js1{std::unique_ptr<Script>(jsi.newScript(QStringLiteral("does-not-exist")))};
	ScriptObject js2{std::unique_ptr<Script>(jsi.newScript(QStringLiteral("script1.js")))};
	ScriptObject js3{std::unique_ptr<Script>(jsi.newScript(QStringLiteral("script2.js")))};

	{
		MockTarget target;
		MockAPI api(&js1, &target);
		QVERIFY(js1.run(api) == false);
	}
	{
		MockTarget target;
		MockAPI api(&js2, &target);
		QVERIFY(js2.run(api));
		QCOMPARE(qobject_cast<MockTarget*>(api.GetTarget())->text, QStringLiteral("It works!"));
		QCOMPARE(api.GetResult(), QVariant(QVariantList({1, 2, 3})));
	}
	{
		MockTarget target;
		MockAPI api(&js3, &target);
		QVERIFY(js3.run(api) == false);
		QCOMPARE(api.GetResult(), QVariant(QStringLiteral("an exception")));
	}
#endif

	ECMAScriptInterface esi(this);

	ScriptObject es1{std::unique_ptr<Script>(esi.newScript(QStringLiteral("does-not-exist")))};
	ScriptObject es2{std::unique_ptr<Script>(esi.newScript(QStringLiteral("script1.js")))};
	ScriptObject es3{std::unique_ptr<Script>(esi.newScript(QStringLiteral("script2.js")))};

	{
		MockTarget target;
		MockAPI api(&es1, &target);
		QVERIFY(es1.run(api) == false);
	}
	{
		MockTarget target;
		MockAPI api(&es2, &target);
		QVERIFY(es2.run(api));
		QCOMPARE(qobject_cast<MockTarget*>(api.GetTarget())->text, QStringLiteral("It works!"));
		QCOMPARE(api.GetResult(), QVariant(QVariantList({1, 2, 3})));
	}
	{
		MockTarget target;
		MockAPI api(&es3, &target);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
		QEXPECT_FAIL("", "Script exceptions are not handled correctly until Qt6", Continue);
#endif
		QVERIFY(es3.run(api) == false);
		QCOMPARE(api.GetResult(), QVariant(QStringLiteral("an exception")));
	}
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestScripting)
