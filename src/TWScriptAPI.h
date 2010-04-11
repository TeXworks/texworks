/*
 This is part of TeXworks, an environment for working with TeX documents
 Copyright (C) 2007-2010  Stefan Löffler & Jonathan Kew
 
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
 see <http://texworks.org/>.
*/

#ifndef TWScriptAPI_H
#define TWScriptAPI_H

#include "TWScript.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>

class TWScriptAPI : public QObject
{
	Q_OBJECT
	
    Q_PROPERTY(QObject* app READ GetApp);
	Q_PROPERTY(QObject* target READ GetTarget);
	Q_PROPERTY(QVariant result READ GetResult WRITE SetResult);
	
public:
	TWScriptAPI(QObject* twapp, QObject* ctx, QVariant& res)
	: m_app(twapp),
	m_target(ctx),
	m_result(res)
	{ }
	
	QObject* GetApp() { return m_app; }
	QObject* GetTarget() { return m_target; }
	QVariant& GetResult() { return m_result; }
	
	void SetResult(const QVariant& rval) { m_result = rval; }
	
public slots:
	// provide utility functions for scripts, implemented as methods on the TW object

	// length of a string in UTF-16 code units, useful if script language uses a different encoding form
	int strlen(const QString& str) const { return str.length(); }
	
	// return the host platform name
	QString platform() const {
#if defined(Q_WS_MAC)
		return QString("MacOSX");
#elif defined(Q_WS_WIN)
		return QString("Windows");
#elif defined(Q_WS_X11)
		return QString("X11");
#else
		return QString("unknown");
#endif
	}

	// QMessageBox functions to display alerts
	int information(QWidget* parent,
					const QString& title, const QString& text,
					int buttons = (int)QMessageBox::Ok,
					int defaultButton = QMessageBox::NoButton) {
		return (int)QMessageBox::information(parent, title, text,
											 (QMessageBox::StandardButtons)buttons,
											 (QMessageBox::StandardButton)defaultButton);
	}
	int question(QWidget* parent,
				 const QString& title, const QString& text,
				 int buttons = (int)QMessageBox::Ok,
				 int defaultButton = QMessageBox::NoButton) {
		return (int)QMessageBox::question(parent, title, text,
										  (QMessageBox::StandardButtons)buttons,
										  (QMessageBox::StandardButton)defaultButton);
	}
	int warning(QWidget* parent,
				const QString& title, const QString& text,
				int buttons = (int)QMessageBox::Ok,
				int defaultButton = QMessageBox::NoButton) {
		return (int)QMessageBox::warning(parent, title, text,
										 (QMessageBox::StandardButtons)buttons,
										 (QMessageBox::StandardButton)defaultButton);
	}
	int critical(QWidget* parent,
				 const QString& title, const QString& text,
				 int buttons = (int)QMessageBox::Ok,
				 int defaultButton = QMessageBox::NoButton) {
		return (int)QMessageBox::critical(parent, title, text,
										  (QMessageBox::StandardButtons)buttons,
										  (QMessageBox::StandardButton)defaultButton);
	}
	
	// QInputDialog functions
	QVariant getInt(QWidget* parent, const QString& title, const QString& label,
					int value = 0, int min = -2147483647, int max = 2147483647, int step = 1) {
		bool ok;
#if QT_VERSION >= 0x040500
		int i = QInputDialog::getInt(parent, title, label, value, min, max, step, &ok);
#else
		int i = QInputDialog::getInteger(parent, title, label, value, min, max, step, &ok);
#endif
		return ok ? QVariant(i) : QVariant();
	}
	QVariant getDouble(QWidget* parent, const QString& title, const QString& label,
					   double value = 0, double min = -2147483647, double max = 2147483647, int decimals = 1) {
		bool ok;
		double d = QInputDialog::getDouble(parent, title, label, value, min, max, decimals, &ok);
		return ok ? QVariant(d) : QVariant();
	}
	QVariant getItem(QWidget* parent, const QString& title, const QString& label,
					 const QStringList& items, int current = 0, bool editable = true) {
		bool ok;
		QString s = QInputDialog::getItem(parent, title, label, items, current, editable, &ok);
		return ok ? QVariant(s) : QVariant();
	}
	QVariant getText(QWidget* parent, const QString& title, const QString& label,
					 const QString& text = QString()) {
		bool ok;
		QString s = QInputDialog::getText(parent, title, label, QLineEdit::Normal, text, &ok);
		return ok ? QVariant(s) : QVariant();
	}
	
	// Allow script to create a QProgressDialog
	QVariant progressDialog(QWidget * parent) {
		QProgressDialog * dlg = new QProgressDialog(parent);
		connect(this, SIGNAL(destroyed(QObject*)), dlg, SLOT(deleteLater()));
		dlg->setCancelButton(NULL);
		dlg->show();
		return QVariant::fromValue(qobject_cast<QWidget*>(dlg));
	}
	
protected:
	QObject* m_app;
	QObject* m_target;
	QVariant& m_result;
};

#endif /* TWScriptAPI_H */
