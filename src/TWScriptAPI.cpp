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

#include "TWScriptAPI.h"

#include <QObject>
#include <QString>
#include <QVariant>
#include <QMessageBox>
#include <QInputDialog>
#include <QProgressDialog>
#include <QCoreApplication>
#include <QUiLoader>
#include <QBuffer>
#include <QDir>

TWScriptAPI::TWScriptAPI(TWScript* script, QObject* twapp, QObject* ctx, QVariant& res)
	: m_script(script),
	  m_app(twapp),
	  m_target(ctx),
	  m_result(res)
{
}
	
void TWScriptAPI::SetResult(const QVariant& rval)
{
	m_result = rval;
}

int TWScriptAPI::strlen(const QString& str) const
{
	return str.length();
}

QString TWScriptAPI::platform() const
{
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

int TWScriptAPI::information(QWidget* parent,
				const QString& title, const QString& text,
				int buttons,
				int defaultButton)
{
	return (int)QMessageBox::information(parent, title, text,
										 (QMessageBox::StandardButtons)buttons,
										 (QMessageBox::StandardButton)defaultButton);
}

int TWScriptAPI::question(QWidget* parent,
			 const QString& title, const QString& text,
			 int buttons,
			 int defaultButton)
{
	return (int)QMessageBox::question(parent, title, text,
									  (QMessageBox::StandardButtons)buttons,
									  (QMessageBox::StandardButton)defaultButton);
}

int TWScriptAPI::warning(QWidget* parent,
			const QString& title, const QString& text,
			int buttons,
			int defaultButton)
{
	return (int)QMessageBox::warning(parent, title, text,
									 (QMessageBox::StandardButtons)buttons,
									 (QMessageBox::StandardButton)defaultButton);
}

int TWScriptAPI::critical(QWidget* parent,
			 const QString& title, const QString& text,
			 int buttons,
			 int defaultButton)
{
	return (int)QMessageBox::critical(parent, title, text,
									  (QMessageBox::StandardButtons)buttons,
									  (QMessageBox::StandardButton)defaultButton);
}

QVariant TWScriptAPI::getInt(QWidget* parent, const QString& title, const QString& label,
				int value, int min, int max, int step)
{
	bool ok;
#if QT_VERSION >= 0x040500
	int i = QInputDialog::getInt(parent, title, label, value, min, max, step, &ok);
#else
	int i = QInputDialog::getInteger(parent, title, label, value, min, max, step, &ok);
#endif
	return ok ? QVariant(i) : QVariant();
}

QVariant TWScriptAPI::getDouble(QWidget* parent, const QString& title, const QString& label,
				   double value, double min, double max, int decimals)
{
	bool ok;
	double d = QInputDialog::getDouble(parent, title, label, value, min, max, decimals, &ok);
	return ok ? QVariant(d) : QVariant();
}

QVariant TWScriptAPI::getItem(QWidget* parent, const QString& title, const QString& label,
				 const QStringList& items, int current, bool editable)
{
	bool ok;
	QString s = QInputDialog::getItem(parent, title, label, items, current, editable, &ok);
	return ok ? QVariant(s) : QVariant();
}

QVariant TWScriptAPI::getText(QWidget* parent, const QString& title, const QString& label,
				 const QString& text)
{
	bool ok;
	QString s = QInputDialog::getText(parent, title, label, QLineEdit::Normal, text, &ok);
	return ok ? QVariant(s) : QVariant();
}
	
void TWScriptAPI::yield()
{
	QCoreApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
}
	
QWidget * TWScriptAPI::progressDialog(QWidget * parent)
{
	QProgressDialog * dlg = new QProgressDialog(parent);
	connect(this, SIGNAL(destroyed(QObject*)), dlg, SLOT(deleteLater()));
	dlg->setCancelButton(NULL);
	dlg->show();
	return dlg;
}
	
QWidget * TWScriptAPI::createUIFromString(const QString& uiSpec, QWidget * parent)
{
	QByteArray ba(uiSpec.toUtf8());
	QBuffer buffer(&ba);
	QUiLoader loader;
	QWidget *widget = loader.load(&buffer, parent);
	if (widget) {
		// ensure that the window is app-modal regardless of what flags might be set
		//! \TODO revisit this when we get asynchronous scripting
		widget->setWindowModality(Qt::ApplicationModal);
		widget->show();
	}
	return widget;
}

QWidget * TWScriptAPI::createUI(const QString& filename, QWidget * parent)
{
	QFileInfo fi(QFileInfo(m_script->getFilename()).absoluteDir(), filename);
	if (!fi.isReadable())
		return NULL;
	QFile file(fi.canonicalFilePath());
	QUiLoader loader;
	QWidget *widget = loader.load(&file, parent);
	if (widget) {
		// ensure that the window is app-modal regardless of what flags might be set
		//! \TODO revisit this when we get asynchronous scripting
		widget->setWindowModality(Qt::ApplicationModal);
		widget->show();
	}
	return widget;
}
	
QWidget * TWScriptAPI::findChildWidget(QWidget* parent, const QString& name)
{
	QWidget* child = parent->findChild<QWidget*>(name);
	return child;
}
	
bool TWScriptAPI::makeConnection(QObject* sender, const QString& signal, QObject* receiver, const QString& slot)
{
	return QObject::connect(sender, QString("2%1").arg(signal).toUtf8().data(),
							receiver, QString("1%1").arg(slot).toUtf8().data());
}
