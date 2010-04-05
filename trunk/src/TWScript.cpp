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

#include "TWScript.h"
#include "TWScriptAPI.h"

#include <QTextStream>
#include <QMetaObject>
#include <QMetaMethod>
#include <QApplication>
#include <QTextCodec>

TWScript::TWScript(TWScriptLanguageInterface *interface, const QString& fileName)
	: m_Interface(interface), m_Filename(fileName), m_Type(ScriptUnknown), m_Enabled(true)
{
}

bool TWScript::run(QObject *context, QVariant& result) const
{
	TWScriptAPI tw(qApp, context, result);
	return execute(&tw);
}

bool TWScript::doParseHeader(const QString& beginComment, const QString& endComment,
							 const QString& Comment, bool skipEmpty /*=true*/)
{
	QFile file(m_Filename);
	QStringList lines;
	QString line;
	
	if (!file.exists() || !file.open(QIODevice::ReadOnly))
		return false;
	
	QTextCodec* codec = QTextCodec::codecForName("UTF-8");
	if (!codec)
		codec = QTextCodec::codecForLocale();
	lines = codec->toUnicode(file.readAll()).split(QRegExp("\r\n|[\n\r]"));
	file.close();
	
	// skip any empty lines
	if (skipEmpty) {
		while (!lines.isEmpty() && lines.first().isEmpty())
			lines.removeFirst();
	}
	if (lines.isEmpty())
		return false;
	
	// is this a valid TW script?
	line = lines.takeFirst();
	if (!beginComment.isEmpty()) {
		if (!line.startsWith(beginComment))
			return false;
		line = line.mid(beginComment.size()).trimmed();
	}
	else if (!Comment.isEmpty()) {
		if (!line.startsWith(Comment))
			return false;
		line = line.mid(Comment.size()).trimmed();
	}
	if (!line.startsWith("TeXworksScript"))
		return false;
	
	// scan to find the extent of the header lines
	QStringList::iterator i;
	for (i = lines.begin(); i != lines.end(); ++i) {
		// have we reached the end?
		if (skipEmpty && i->isEmpty()) {
			i = lines.erase(i);
			--i;
			continue;
		}
		if (!endComment.isEmpty()) {
			if (i->startsWith(endComment))
				break;
		}
		if (!i->startsWith(Comment))
			break;
		*i = i->mid(Comment.size()).trimmed();
	}
	lines.erase(i, lines.end());
	
	return doParseHeader(lines);
}

bool TWScript::doParseHeader(const QStringList & lines)
{
	QString line, key, value;
	
	foreach (line, lines) {
		key = line.section(':', 0, 0).trimmed();
		value = line.section(':', 1).trimmed();
		
		if (key == "Title") m_Title = value;
		else if (key == "Description") m_Description = value;
		else if (key == "Author") m_Author = value;
		else if (key == "Version") m_Version = value;
		else if (key == "Script-Type") {
			if (value == "hook") m_Type = ScriptHook;
			else if (value == "standalone") m_Type = ScriptStandalone;
			else m_Type = ScriptUnknown;
		}
		else if (key == "Hook") m_Hook = value;
		else if (key == "Context") m_Context = value;
		else if (key == "Shortcut") m_KeySequence = QKeySequence(value);
	}
	
	return m_Type != ScriptUnknown && !m_Title.isEmpty();
}

/*static*/
TWScript::PropertyResult TWScript::doGetProperty(const QObject * obj, const QString& name, QVariant & value)
{
	int iProp, i;
	QMetaProperty prop;
	
	if (!obj || !(obj->metaObject())) return Property_Invalid;
	
	// Get the parameters
	iProp = obj->metaObject()->indexOfProperty(qPrintable(name));
	
	// if we didn't find a property maybe it's a method
	if (iProp < 0) {
		for (i = 0; i < obj->metaObject()->methodCount(); ++i) {
			if (QString(obj->metaObject()->method(i).signature()).startsWith(name + "("))
				return Property_Method;
		}
		return Property_DoesNotExist;
	}
	
	prop = obj->metaObject()->property(iProp);
	
	// If we can't get the property's value, abort
	if (!prop.isReadable())
		return Property_NotReadable;
	
	value = prop.read(obj);
	return Property_OK;
}

/*static*/
TWScript::PropertyResult TWScript::doSetProperty(QObject * obj, const QString& name, const QVariant & value)
{
	int iProp;
	QMetaProperty prop;
	
	if (!obj || !(obj->metaObject())) return Property_Invalid;
	
	iProp = obj->metaObject()->indexOfProperty(qPrintable(name));
	
	// if we didn't find the property abort
	if (iProp < 0) return Property_DoesNotExist;
	
	prop = obj->metaObject()->property(iProp);
	
	// If we can't set the property's value, abort
	if (!prop.isWritable()) return Property_NotWritable;
	
	prop.write(obj, value);
	return Property_OK;
}

/*static*/
TWScript::MethodResult TWScript::doCallMethod(QObject * obj, const QString& name,
											  QVariantList & arguments, QVariant & result)
{
	const QMetaObject * mo;
	bool methodExists = false;
	QList<QGenericArgument> genericArgs;
	int type, i, j;
	QString typeName;
	char * strTypeName;
	QMetaMethod mm;
	QGenericReturnArgument retValArg;
	void * retValBuffer = NULL;
	TWScript::MethodResult status;
	
	if (!obj || !(obj->metaObject())) return Method_Invalid;
	
	mo = obj->metaObject();
	
	for (i = 0; i < mo->methodCount(); ++i) {
		mm = mo->method(i);
		// Check for the method name
		if (!QString(mm.signature()).startsWith(name + "(")) continue;
		// we can only call public methods
		if (mm.access() != QMetaMethod::Public) continue;
		
		methodExists = true;
		
		// we need the correct number of arguments
		if (mm.parameterTypes().count() != arguments.count()) continue;
		
		// Check if the given arguments are compatible with those taken by the
		// method
		for (j = 0; j < arguments.count(); ++j) {
			type = QMetaType::type(mm.parameterTypes()[j]);
			int typeOfArg = (int)arguments[j].type();
			if (typeOfArg != (int)type)
				if (!arguments[j].canConvert((QVariant::Type)type)) break;
		}
		if (j < arguments.count()) continue;
		
		// Convert the arguments into QGenericArgument structures
		for (j = 0; j < arguments.count() && j < 10; ++j) {
			typeName = mm.parameterTypes()[j];
			type = QMetaType::type(qPrintable(typeName));
			
			// allocate type name on the heap so it survives the method call
			strTypeName = new char[typeName.size() + 1];
			strcpy(strTypeName, qPrintable(typeName));
			
			arguments[j].convert((QVariant::Type)type);
			// \TODO	handle failure during conversion
			
			// Note: This line is a hack!
			// QVariant::data() is undocumented; QGenericArgument should not be
			// called directly; if this ever causes problems, think of another
			// (better) way to do this
			genericArgs.append(QGenericArgument(strTypeName, arguments[j].data()));
		}
		// Fill up the list so we get the 10 values we need later on
		for (; j < 10; ++j)
			genericArgs.append(QGenericArgument());
		
		typeName = mm.typeName();
		if (typeName.isEmpty()) {
			// no return type
			retValArg = QGenericReturnArgument();
		}
		else if (typeName == "QVariant") {
			// QMetaType can't construct QVariant objects
			retValArg = Q_RETURN_ARG(QVariant, result);
		}
		else {
			// Note: These two lines are a hack!
			// QGenericReturnArgument should not be constructed directly; if
			// this ever causes problems, think of another (better) way to do this
			retValBuffer = QMetaType::construct(QMetaType::type(mm.typeName()));
			retValArg = QGenericReturnArgument(mm.typeName(), retValBuffer);
		}
		
		if (mo->invokeMethod(obj, qPrintable(name),
							 Qt::DirectConnection,
							 retValArg,
							 genericArgs[0],
							 genericArgs[1],
							 genericArgs[2],
							 genericArgs[3],
							 genericArgs[4],
							 genericArgs[5],
							 genericArgs[6],
							 genericArgs[7],
							 genericArgs[8],
							 genericArgs[9])
		   ) {
			if (retValBuffer)
				result = QVariant(QMetaType::type(mm.typeName()), retValBuffer);
			else if (typeName == "QVariant")
				; // don't do anything here; the return valus is already in result
			else
				result = QVariant();
			status = Method_OK;
		}
		else status = Method_Failed;
		
		if (retValBuffer) QMetaType::destroy(QMetaType::type(mm.typeName()), retValBuffer);
		
		for (j = 0; j < arguments.count() && j < 10; ++j) {
			// we pushed the data on the heap, we need to remove it from there
			delete[] genericArgs[j].name();
		}
		
		return status;
	}
	
	if (methodExists) return Method_WrongArgs;
	return Method_DoesNotExist;
}
