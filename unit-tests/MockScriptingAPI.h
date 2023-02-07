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

#include "scripting/Script.h"
#include "scripting/ScriptAPIInterface.h"

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
	MockTarget * _target;

	Q_PROPERTY(QObject* app READ GetApp)
	Q_PROPERTY(QObject* target READ GetTarget)
	Q_PROPERTY(QVariant result READ GetResult WRITE SetResult)
	Q_PROPERTY(QObject * script READ GetScript)

public:
	MockAPI(Script * script, MockTarget * target) : _script(script), _target(target) { }
	QObject* clone() const override { return new MockAPI(_script, _target); }

	QObject* self() override { return this; }

	QObject* GetApp() override { return this; }
	QObject* GetTarget() override { return _target; }
	QObject* GetScript() override { return _script; }
	QVariant& GetResult() override { return _result; }
	void SetResult(const QVariant& rval) override { _result = rval; }
	int strlen(const QString& str) const override { return static_cast<int>(str.length()); }
	QString platform() const override { return QString(); }
	int getQtVersion() const override { return QT_VERSION; }
	QMap<QString, QVariant> system(const QString& cmdline, bool waitForResult = true) override {
		Q_UNUSED(cmdline);
		Q_UNUSED(waitForResult);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), 0},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
			{QStringLiteral("output"), QString()}
		};
	}
	QMap<QString, QVariant> launchFile(const QString& fileName) const override {
		Q_UNUSED(fileName);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
	}
	int writeFile(const QString& filename, const QString& content) const override {
		Q_UNUSED(filename);
		Q_UNUSED(content);
		return ScriptAPIInterface::SystemAccess_Failed;
	}
	// Content is read in text-mode in utf8 encoding
	QMap<QString, QVariant> readFile(const QString& filename) const override {
		Q_UNUSED(filename);
		return {
			{QStringLiteral("status"), ScriptAPIInterface::SystemAccess_Failed},
			{QStringLiteral("result"), QString()},
			{QStringLiteral("message"), QStringLiteral("This is only a MockAPI")},
		};
	}
	int fileExists(const QString& filename) const override {
		Q_UNUSED(filename);
		return ScriptAPIInterface::SystemAccess_Failed;
	}

	// QMessageBox functions to display alerts
	int information(QWidget* parent,
							const QString& title, const QString& text,
							int buttons = QMessageBox::Ok,
							int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	int question(QWidget* parent,
						 const QString& title, const QString& text,
						 int buttons = QMessageBox::Ok,
						 int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	int warning(QWidget* parent,
						const QString& title, const QString& text,
						int buttons = QMessageBox::Ok,
						int defaultButton = QMessageBox::NoButton) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(text);
		Q_UNUSED(buttons);
		Q_UNUSED(defaultButton);
		return QMessageBox::NoButton;
	}
	int critical(QWidget* parent,
						 const QString& title, const QString& text,
						 int buttons = QMessageBox::Ok,
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
	QVariant getInt(QWidget* parent, const QString& title, const QString& label,
							int value = 0, int min = -2147483647, int max = 2147483647, int step = 1) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(min);
		Q_UNUSED(max);
		Q_UNUSED(step);
		return value;
	}
	QVariant getDouble(QWidget* parent, const QString& title, const QString& label,
							   double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(min);
		Q_UNUSED(max);
		Q_UNUSED(decimals);
		return value;
	}
	QVariant getItem(QWidget* parent, const QString& title, const QString& label,
							 const QStringList& items, int current = 0, bool editable = true) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		Q_UNUSED(editable);
		return items[current];
	}
	QVariant getText(QWidget* parent, const QString& title, const QString& label,
							 const QString& text = QString()) override {
		Q_UNUSED(parent);
		Q_UNUSED(title);
		Q_UNUSED(label);
		return text;
	}
	void yield() override {}
	QWidget * progressDialog(QWidget * parent) override {
		Q_UNUSED(parent);
		return nullptr;
	}
	QWidget * createUIFromString(const QString& uiSpec, QWidget * parent = nullptr) override {
		Q_UNUSED(uiSpec);
		Q_UNUSED(parent);
		return nullptr;
	}
	QWidget * createUI(const QString& filename, QWidget * parent = nullptr) override {
		Q_UNUSED(filename);
		Q_UNUSED(parent);
		return nullptr;
	}
	// to find children of a widget
	QWidget * findChildWidget(QWidget* parent, const QString& name) override {
		Q_UNUSED(parent);
		Q_UNUSED(name);
		return nullptr;
	}
	bool makeConnection(QObject* sender, const QString& signal, QObject* receiver, const QString& slot) override {
		return connect(sender, qPrintable(signal), receiver, qPrintable(slot));
	}
	QMap<QString, QVariant> getDictionaryList(const bool forceReload = false) override {
		Q_UNUSED(forceReload);
		return {};
	}
	QList<QVariant> getEngineList() const override { return {}; }

	bool mayExecuteSystemCommand(const QString& cmd, QObject * context) const override {
		Q_UNUSED(cmd);
		Q_UNUSED(context);
		return false;
	}
	bool mayWriteFile(const QString& filename, QObject * context) const  override {
		Q_UNUSED(filename);
		Q_UNUSED(context);
		return false;
	}
	bool mayReadFile(const QString& filename, QObject * context) const  override {
		Q_UNUSED(filename);
		Q_UNUSED(context);
		return false;
	}
};

} // namespace UnitTest
