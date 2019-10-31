#include "scripting/ScriptAPIInterface.h"
#include "scripting/Script.h"
#include <QObject>

using namespace Tw::Scripting;

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

} // namespace UnitTest
