/*
 This is part of TeXworks, an environment for working with TeX documents
 Copyright (C) 2007-09  Stefan Löffler & Jonathan Kew
 
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

#include "TWLuaPlugin.h"

#include <QCoreApplication>
#include <QTextStream>
#include <QtPlugin>
#include <QMetaObject>
#include <QStringList>

TWLuaPlugin::TWLuaPlugin()
{
	// Base class constructor
	QObject::QObject();
	
	// Initialize lua state
	luaState = luaL_newstate();
	if (luaState) {
		luaL_openlibs(luaState);
	}
}

TWLuaPlugin::~TWLuaPlugin()
{
	if (luaState) lua_close(luaState);
}

TWScript* TWLuaPlugin::newScript(const QString& fileName)
{
	return new LuaScript(this, fileName);
}

Q_EXPORT_PLUGIN2(TWLuaPlugin, TWLuaPlugin)


bool LuaScript::run(QObject *context, QVariant& result) const
{
	int status;
	lua_State * L = m_LuaPlugin->getLuaState();

	if (!L) return false;

	// register the context for use in lua
	if (!LuaScript::pushQObject(L, context, false)) {
		result = tr("Could not register TWTarget");
		return false;
	}
	lua_setglobal(L, "TWTarget");
	
	// register the app object for use in lua
	if (!LuaScript::pushQObject(L, QCoreApplication::instance(), false)) {
		result = tr("Could not register TWApp");
		return false;
	}
	lua_setglobal(L, "TWApp");
	
	// register the global result variable
	lua_pushnil(L);
	lua_setglobal(L, "result");
	
	status = luaL_loadfile(L, qPrintable(m_Filename));
	if (status != 0) {
		result = getLuaStackValue(L, -1, false).toString();
		lua_pop(L, 1);
		return false;
	}

	// call the script
	status = lua_pcall(L, 0, LUA_MULTRET, 0);
	if (status != 0) {
		result = getLuaStackValue(L, -1, false).toString();
		lua_pop(L, 1);
		return false;
	}
	
	// see if we have a return value
	lua_getglobal(L, "result");
	if (!lua_isnil(L, -1)) {
		result = getLuaStackValue(L, -1, false);
	}
	
	// pop the return value from the stack
	lua_pop(L, 1);

	lua_pushnil(L);
	lua_setglobal(L, "TWTarget");
	lua_pushnil(L);
	lua_setglobal(L, "TWApp");
	lua_pushnil(L);
	lua_setglobal(L, "result");

	return true;
}

/*static*/
int LuaScript::pushQObject(lua_State * L, QObject * obj, const bool throwError /*= true*/)
{
	if (!L || !obj) return 0;
	
	lua_newtable(L);

	// register callback for all get/set operations on object properties and
	// all call operations on object methods
	if (lua_getmetatable(L, -1) == 0)
		lua_newtable(L);
	
	lua_pushlightuserdata(L, obj);
	lua_pushcclosure(L, LuaScript::setProperty, 1);
	lua_setfield(L, -2, "__newindex");

	lua_pushlightuserdata(L, obj);
	lua_pushcclosure(L, LuaScript::getProperty, 1);
	lua_setfield(L, -2, "__index");

	lua_pushlightuserdata(L, obj);
	lua_pushcclosure(L, LuaScript::callMethod, 1);
	lua_setfield(L, -2, "__call");

	lua_setmetatable(L, -2);
	return 1;
}

/*static*/
int LuaScript::pushVariant(lua_State * L, const QVariant & v, const bool throwError /*= true*/)
{
	int i;
	QVariantList::const_iterator iList;
	QVariantList list;
	QVariantHash::const_iterator iHash;
	QVariantHash hash;
	QVariantMap::const_iterator iMap;
	QVariantMap map;

	if (!L) return 0;
	if (v.isNull()) {
		lua_pushnil(L);
		return 1;
	}
	
	switch (v.type()) {
		case QVariant::Bool:
			lua_pushboolean(L, v.toBool());
			return 1;
		case QVariant::Double:
		case QVariant::Int:
		case QVariant::LongLong:
		case QVariant::UInt:
		case QVariant::ULongLong:
			lua_pushnumber(L, v.toDouble());
			return 1;
		case QVariant::Char:
		case QVariant::String:
			lua_pushstring(L, qPrintable(v.toString()));
			return 1;
		case QVariant::List:
		case QVariant::StringList:
			list = v.toList();

			lua_newtable(L);
			for (i = 1, iList = list.begin(); iList != list.end(); ++iList, ++i) {
				LuaScript::pushVariant(L, *iList);
				lua_setfield(L, -2, qPrintable(QString("%1").arg(i)));
			}
			return 1;
		case QVariant::Hash:
			hash = v.toHash();
			
			lua_newtable(L);
			for (iHash = hash.begin(); iHash != hash.end(); ++iHash) {
				LuaScript::pushVariant(L, iHash.value());
				lua_setfield(L, -2, qPrintable(iHash.key()));
			}
			return 1;
		case QVariant::Map:
			map = v.toMap();
			
			lua_newtable(L);
			for (iMap = map.begin(); iMap != map.end(); ++iMap) {
				LuaScript::pushVariant(L, iMap.value());
				lua_setfield(L, -2, qPrintable(iMap.key()));
			}
			return 1;
		case QMetaType::QObjectStar:
		case QMetaType::QWidgetStar:
			return LuaScript::pushQObject(L, v.value<QObject*>(), throwError);
		default:
			// Don't throw errors if we are not in protected mode in lua, i.e.
			// if the call to this function originated from C code, not in response
			// to a lua request (e.g. during initialization or finalization) as that
			// would crash Tw
			if (throwError) luaL_error(L, "the type %s is currently not supported", v.typeName());
	}
	return 0;
}

/*static*/
int LuaScript::getProperty(lua_State * L)
{
	QObject * obj;
	QString propName;
	QVariant result;

	// We should have the lua table (=object) we're called from and the property
	// name we should get; if not, something is wrong
	if (lua_gettop(L) != 2) {
		luaL_error(L, qPrintable(tr("__get: invalid call -- expected exactly 2 arguments, got %f")), lua_gettop(L));
		return 0;
	}

	// Get the QObject* we operate on
	obj = (QObject*)lua_topointer(L, lua_upvalueindex(1));
	
	// Get the parameters
	propName = lua_tostring(L, 2);
	
	switch (doGetProperty(obj, propName, result)) {
		case Property_DoesNotExist:
			luaL_error(L, qPrintable(tr("__get: object doesn't have property/method %s")), qPrintable(propName));
			return 0;
		case Property_NotReadable:
			luaL_error(L, qPrintable(tr("__get: property %s is not readable")), qPrintable(propName));
			return 0;
		case Property_Method:
			lua_pushlightuserdata(L, obj);
			lua_pushstring(L, qPrintable(propName));
			lua_pushcclosure(L, LuaScript::callMethod, 2);
			return 1;
		case Property_OK:
			return LuaScript::pushVariant(L, result);
		default:
			break;
	}
	// we should never reach this point
	return 0;

}

/*static*/
int LuaScript::callMethod(lua_State * L)
{
	int i;
	QObject * obj;
	QString methodName;
	QList<QVariant> args;
	QVariant result;

	// Get the QObject* we operate on
	obj = (QObject*)lua_topointer(L, lua_upvalueindex(1));

	methodName = lua_tostring(L, lua_upvalueindex(2));
	
	for (i = 1; i <= lua_gettop(L); ++i) {
		args.append(getLuaStackValue(L, i));
	}

	switch (doCallMethod(obj, methodName, args, result)) {
		case Method_OK:
			return LuaScript::pushVariant(L, result);
		case Method_DoesNotExist:
			luaL_error(L, qPrintable(tr("__call: the method %s doesn't exist")), qPrintable(methodName));
			return 0;
		case Method_WrongArgs:
			luaL_error(L, qPrintable(tr("__call: couldn't call %s with the given arguments")), qPrintable(methodName));
			return 0;
		case Method_Failed:
			luaL_error(L, qPrintable(tr("__call: internal error while executing %s")), qPrintable(methodName));
			return 0;
		default:
			break;
	}
	
	// we should never reach this point
	return 0;
}

/*static*/
int LuaScript::setProperty(lua_State * L)
{
	QObject * obj;
	QString propName;

	// We should have the lua table (=object) we're called from, the property
	// name we should set, and the new value; if not, something is wrong
	if (lua_gettop(L) != 3) {
		luaL_error(L, qPrintable(tr("__set: invalid call -- expected exactly 3 arguments, got %f")), lua_gettop(L));
		return 0;
	}

	// Get the QObject* we operate on
	obj = (QObject*)lua_topointer(L, lua_upvalueindex(1));

	// Get the parameters
	propName = lua_tostring(L, 2);

	switch (doSetProperty(obj, propName, LuaScript::getLuaStackValue(L, 3))) {
		case Property_DoesNotExist:
			luaL_error(L, qPrintable(tr("__set: object doesn't have property %s")), qPrintable(propName));
			return 0;
		case Property_NotWritable:
			luaL_error(L, qPrintable(tr("__set: property %s is not writable")), qPrintable(propName));
			return 0;
		case Property_OK:
			return 0;
		default:
			break;
	}
	// we should never reach this point
	return 0;
}

/*static*/
QVariant LuaScript::getLuaStackValue(lua_State * L, int idx, const bool throwError /*= true*/)
{
	bool isArray = true, isHash = true;
	QVariantList vl;
	QVariantHash vh;
	int i, n, iMax;

	if (!L) return QVariant();
	
	switch (lua_type(L, idx)) {
		case LUA_TNIL:
			return QVariant();
		case LUA_TNUMBER:
			return QVariant(lua_tonumber(L, idx));
		case LUA_TBOOLEAN:
			return QVariant(lua_toboolean(L, idx) == 1);
		case LUA_TSTRING:
			return QVariant(lua_tostring(L, idx));
		case LUA_TTABLE:
			// Special treatment for tables
			// If all keys are in the form 1..n, we can convert it to a QList
			
			// convert index to an absolute value since we'll be messing with
			// the stack
			if (idx < 0) idx += lua_gettop(L) + 1;
			
			// taken from the lua reference of lua_next()
			lua_pushnil(L);
			n = 0;
			iMax = 0;
			while (lua_next(L, idx)) {
				if (isArray) {
					if (!lua_isnumber(L, -2)) isArray = false;
					else {
						++n;
						if (lua_tonumber(L, -2) > iMax) iMax = lua_tonumber(L, -2);
					}
				}
				if (isHash) {
					// keys must be convertable to string
					if (!lua_isstring(L, -2)) isHash = false;
					// some value types are not supported by QVariant
					if (
						lua_isfunction(L, -1) ||
						lua_islightuserdata(L, -1) ||
						lua_isthread(L, -1) ||
						lua_isuserdata(L, -1)
					) isHash = false;
				}
				lua_pop(L, 1);
				++i;
			}
			if (n != iMax) isArray = false;
			
			if (isArray) {
				for (i = 1; i <= n; ++i) {
					lua_getfield(L, idx, qPrintable(QString("%1").arg(i)));
					vl.append(LuaScript::getLuaStackValue(L, -1));
					lua_pop(L, 1);
				}
				return vl;
			}
			// we have no way to distinguish between QHash and QMap, so I
			// arbitrarly chose the former
			if (isHash) {
				lua_pushnil(L);
				while (lua_next(L, idx)) {
					// duplicate the key. If we didn't, lua_tostring could
					// convert it, thereby confusing lua_next later on
					lua_pushvalue(L, -2);
					vh.insert(lua_tostring(L, -1), LuaScript::getLuaStackValue(L, -2));
					lua_pop(L, 2);
				}
				return vh;
			}
			
			// deliberately no break here; if the table could not be converted
			// to QList or QHash, we have to treat it as unsupported
		case LUA_TFUNCTION:
		case LUA_TUSERDATA:
		case LUA_TTHREAD:
		case LUA_TLIGHTUSERDATA:
		default:
			// Don't throw errors if we are not in protected mode in lua, i.e.
			// if the call to this function originated from C code, not in response
			// to a lua request (e.g. during initialization or finalization) as that
			// would crash Tw
			if (throwError) luaL_error(L, qPrintable(tr("the lua type %s is currently not supported")), lua_typename(L, lua_type(L, idx)));
	}
	return QVariant();
}
