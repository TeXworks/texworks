/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019  Stefan Löffler

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
#include "scripting/JSScriptInterface.h"
#include "scripting/JSScript.h"

using namespace Tw::Scripting;

Q_DECLARE_METATYPE(QSharedPointer<Script>)
Q_DECLARE_METATYPE(Script::ScriptType)

namespace UnitTest {

class MockTarget : public QObject
{
	Q_OBJECT
public:
	QString text;
	Q_INVOKABLE void insertText(const QString& text) { this->text.append(text); }
};

class MockAPI : public QObject, public ScriptAPIInterface
{
	Q_OBJECT
	Script * _script;
	QVariant _result;
	MockTarget _target;

	Q_PROPERTY(QObject* app READ GetApp)
	Q_PROPERTY(QObject* target READ GetTarget)
	Q_PROPERTY(QVariant result READ GetResult WRITE SetResult)
	Q_PROPERTY(QObject * script READ GetScript)

public:
	MockAPI(Script * script) : _script(script) { }

	virtual QObject* self() override { return this; }

	virtual QObject* GetApp() override { return this; }
	virtual QObject* GetTarget() override { return &_target; }
	virtual QObject* GetScript() override { return _script; }
	virtual QVariant& GetResult() override { return _result; }
	virtual void SetResult(const QVariant& rval) override { _result = rval; }
	virtual int strlen(const QString& str) const override { return str.length(); }
	virtual QString platform() const override { return QString(); }
	virtual int getQtVersion() const override { return QT_VERSION; }
	virtual QMap<QString, QVariant> system(const QString& cmdline, bool waitForResult = true) override {
		Q_UNUSED(cmdline);
		Q_UNUSED(waitForResult);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), 0},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
			{QStringLiteral("output"), QString()}
		};
	}
	virtual QMap<QString, QVariant> launchFile(const QString& fileName) const override {
		Q_UNUSED(fileName);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
	}
	virtual int writeFile(const QString& filename, const QString& content) const override {
		Q_UNUSED(filename);
		Q_UNUSED(content);
		return ScriptAPIInterface::SystemAccess_Failed;
	}
	// Content is read in text-mode in utf8 encoding
	virtual QMap<QString, QVariant> readFile(const QString& filename) const override {
		Q_UNUSED(filename);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), QString()},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
	}
	virtual int fileExists(const QString& filename) const override {
		Q_UNUSED(filename);
		return ScriptAPIInterface::SystemAccess_Failed;
	}

	// QMessageBox functions to display alerts
	virtual int information(QWidget* parent,
							const QString& title, const QString& text,
							int buttons = (int)QMessageBox::Ok,
							int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	virtual int question(QWidget* parent,
						 const QString& title, const QString& text,
						 int buttons = (int)QMessageBox::Ok,
						 int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	virtual int warning(QWidget* parent,
						const QString& title, const QString& text,
						int buttons = (int)QMessageBox::Ok,
						int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	virtual int critical(QWidget* parent,
						 const QString& title, const QString& text,
						 int buttons = (int)QMessageBox::Ok,
						 int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}

	// QInputDialog functions
	// These return QVariant rather than simple types, so that they can return null
	// to indicate that the dialog was cancelled.
	virtual QVariant getInt(QWidget* parent, const QString& title, const QString& label,
							int value = 0, int min = -2147483647, int max = 2147483647, int step = 1) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(min);
		Q_UNUSED(max);
		Q_UNUSED(step);
		return value;
	}
	virtual QVariant getDouble(QWidget* parent, const QString& title, const QString& label,
							   double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(min);
		Q_UNUSED(max);
		Q_UNUSED(decimals);
		return value;
	}
	virtual QVariant getItem(QWidget* parent, const QString& title, const QString& label,
							 const QStringList& items, int current = 0, bool editable = true) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(editable);
		return items[current];
	}
	virtual QVariant getText(QWidget* parent, const QString& title, const QString& label,
							 const QString& text = QString()) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		return text;
	}
	virtual void yield() override {}
	virtual QWidget * progressDialog(QWidget * parent) override {
		Q_UNUSED(parent);
		return nullptr;
	}
	virtual QWidget * createUIFromString(const QString& uiSpec, QWidget * parent = nullptr) override {
		Q_UNUSED(uiSpec);
		Q_UNUSED(parent);
		return nullptr;
	}
	virtual QWidget * createUI(const QString& filename, QWidget * parent = nullptr) override {
		Q_UNUSED(filename);
		Q_UNUSED(parent);
		return nullptr;
	}
	// to find children of a widget
	virtual QWidget * findChildWidget(QWidget* parent, const QString& name) override {
		Q_UNUSED(parent);
		Q_UNUSED(name);
		return nullptr;
	}
	virtual bool makeConnection(QObject* sender, const QString& signal, QObject* receiver, const QString& slot) override {
		return connect(sender, qPrintable(signal), receiver, qPrintable(slot));
	}
	virtual QMap<QString, QVariant> getDictionaryList(const bool forceReload = false) override {
		Q_UNUSED(forceReload);
		return {};
	}
	virtual QList<QVariant> getEngineList() const override { return {}; }

	virtual bool mayExecuteSystemCommand(const QString& cmd, QObject * context) const override {
		Q_UNUSED(cmd);
		Q_UNUSED(context);
		return false;
	}
	virtual bool mayWriteFile(const QString& filename, QObject * context) const  override {
		Q_UNUSED(filename);
		Q_UNUSED(context);
		return false;
	}
	virtual bool mayReadFile(const QString& filename, QObject * context) const  override {
		Q_UNUSED(filename);
		Q_UNUSED(context);
		return false;
	}
};

void TestScripting::scriptLanguageName()
{
	Tw::Scripting::JSScriptInterface js;
	QCOMPARE(js.scriptLanguageName(), QStringLiteral("QtScript"));
}

void TestScripting::scriptLanguageURL()
{
	Tw::Scripting::JSScriptInterface js;
	QCOMPARE(js.scriptLanguageURL(), QStringLiteral("http://doc.qt.io/qt-5/qtscript-index.html"));
}

void TestScripting::canHandleFile()
{
	Tw::Scripting::JSScriptInterface js;
	QFileInfo fiNull;
	QFileInfo fiJs(QStringLiteral("file.js"));
	QFileInfo fiPy(QStringLiteral("file.py"));
	QFileInfo fiLua(QStringLiteral("file.lua"));
	QFileInfo fiTex(QStringLiteral("file.tex"));

	QVERIFY(js.canHandleFile(fiNull) == false);
	QVERIFY(js.canHandleFile(fiJs));
	QVERIFY(js.canHandleFile(fiPy) == false);
	QVERIFY(js.canHandleFile(fiLua) == false);
	QVERIFY(js.canHandleFile(fiTex) == false);
}

void TestScripting::isEnabled()
{
	JSScriptInterface jsi;
	Script * js = jsi.newScript(QString());

	QVERIFY(js->isEnabled());
	js->setEnabled(false);
	QVERIFY(js->isEnabled() == false);
	js->setEnabled(true);
	QVERIFY(js->isEnabled());
}

void TestScripting::getScriptLanguagePlugin()
{
	JSScriptInterface jsi;

	QCOMPARE(jsi.newScript(QString())->getScriptLanguagePlugin(), &jsi);
}

void TestScripting::getFilename()
{
	JSScriptInterface jsi;
	QString invalid = QStringLiteral("does-not-exist");

	QCOMPARE(jsi.newScript(QString())->getFilename(), QString());
	QCOMPARE(jsi.newScript(invalid)->getFilename(), invalid);
}

void TestScripting::globals()
{
	JSScriptInterface jsi;
	QSharedPointer<Script> js = QSharedPointer<Script>(jsi.newScript(QString()));

	QString key = QStringLiteral("myGlobal");
	QVariant val(42.f);

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

	JSScriptInterface jsi;

	QTest::newRow("invalid") << QSharedPointer<Script>(jsi.newScript(QString()))
							 << false
							 << Script::ScriptType::ScriptUnknown
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QString()
							 << QKeySequence();
	QTest::newRow("script1.js") << QSharedPointer<Script>(jsi.newScript(QStringLiteral("script1.js")))
								<< true
								<< Script::ScriptType::ScriptStandalone
								<< QStringLiteral("Tw.insertText test")
								<< QStringLiteral(u"This is a unicode string 🤩")
								<< QStringLiteral(u"Stefan Löffler")
								<< QStringLiteral("0.0.1")
								<< QString()
								<< QStringLiteral("TeXDocument")
								<< QKeySequence(QStringLiteral("Ctrl+Alt+Shift+I"));
	QTest::newRow("script2.js") << QSharedPointer<Script>(jsi.newScript(QStringLiteral("script2.js")))
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
	Script * s = JSScriptInterface().newScript(QString());
	MockAPI api(s);

	QCOMPARE(api.self(), &api);
	QCOMPARE(api.GetApp(), &api);
	QCOMPARE(api.GetScript(), s);

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
	QCOMPARE(api.writeFile(QString(), QString()), ScriptAPIInterface::SystemAccess_Failed);
	{
		QMap<QString, QVariant> retVal = {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), QString()},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
		QCOMPARE(api.readFile(QString()), retVal);
	}
	QCOMPARE(api.fileExists(QString()), ScriptAPIInterface::SystemAccess_Failed);
	QCOMPARE(api.information(nullptr, QString(), QString()), QMessageBox::NoButton);
	QCOMPARE(api.question(nullptr, QString(), QString()), QMessageBox::NoButton);
	QCOMPARE(api.warning(nullptr, QString(), QString()), QMessageBox::NoButton);
	QCOMPARE(api.critical(nullptr, QString(), QString()), QMessageBox::NoButton);
	QCOMPARE(api.getInt(nullptr, QString(), QString(), 42), 42);
	QCOMPARE(api.getDouble(nullptr, QString(), QString(), 42.3), 42.3);
	QCOMPARE(api.getItem(nullptr, QString(), QString(), {QStringLiteral("1"), QStringLiteral("2")}, 1), QStringLiteral("2"));
	QCOMPARE(api.getText(nullptr, QString(), QString(), QStringLiteral("42.3")), QStringLiteral("42.3"));
	api.yield();
	QCOMPARE(api.progressDialog(nullptr), nullptr);
	QCOMPARE(api.createUIFromString(QString()), nullptr);
	QCOMPARE(api.createUI(QString()), nullptr);
	QCOMPARE(api.findChildWidget(nullptr, QString()), nullptr);
	QVERIFY(api.makeConnection(nullptr, QString(), nullptr, QString()) == false);
	QCOMPARE(api.getDictionaryList(), {});
	QCOMPARE(api.getEngineList(), {});
	QVERIFY(api.mayExecuteSystemCommand(QString(), nullptr) == false);
	QVERIFY(api.mayWriteFile(QString(), nullptr) == false);
	QVERIFY(api.mayReadFile(QString(), nullptr) == false);

	delete s;
}

void TestScripting::execute()
{
	JSScriptInterface jsi;

	QSharedPointer<Script> js1 = QSharedPointer<Script>(jsi.newScript(QStringLiteral("does-not-exist")));
	QSharedPointer<Script> js2 = QSharedPointer<Script>(jsi.newScript(QStringLiteral("script1.js")));
	QSharedPointer<Script> js3 = QSharedPointer<Script>(jsi.newScript(QStringLiteral("script2.js")));

	{
		MockAPI api(js1.data());
		QVERIFY(js1->run(api) == false);
	}
	{
		MockAPI api(js2.data());
		QVERIFY(js2->run(api));
		QCOMPARE(qobject_cast<MockTarget*>(api.GetTarget())->text, QStringLiteral("It works!"));
		QCOMPARE(api.GetResult(), QVariant(QVariantList({1, 2, 3})));
	}
	{
		MockAPI api(js3.data());
		QVERIFY(js3->run(api) == false);
		QCOMPARE(api.GetResult(), QVariant(QStringLiteral("an exception")));
	}
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

QTEST_MAIN(UnitTest::TestScripting)

#include "Scripting_test.moc"
