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

#include "TWPythonPlugin.h"

#include <QCoreApplication>
#include <QtPlugin>
#include <QMetaObject>
#include <QStringList>
#include <QTextStream>

/* macros that may not be available in older python headers */
#ifndef Py_RETURN_NONE
#define Py_RETURN_NONE return Py_INCREF(Py_None), Py_None
#endif
#ifndef Py_RETURN_TRUE
#define Py_RETURN_TRUE return Py_INCREF(Py_True), Py_True
#endif
#ifndef Py_RETURN_FALSE
#define Py_RETURN_FALSE return Py_INCREF(Py_False), Py_False
#endif

/* Py_ssize_t is new in Python 2.5 */
#if PY_VERSION_HEX < 0x02050000
typedef int Py_ssize_t;
#endif

/** \brief	Structure to hold data for the pyQObject wrapper */
typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
	PyObject * _TWcontext;	///< pointer to the QObject wrapped by this object
} pyQObject;

/** \brief	Structure to hold data for the pyQObjectMethodObject wrapper */
typedef struct {
	PyObject_HEAD
	/* Type-specific fields go here. */
	PyObject * _TWcontext;	///< pointer to the QObject the method wrapped by this object belongs to
	PyObject * _methodName;	///< string describing the method name wrapped by this object
} pyQObjectMethodObject;
static PyTypeObject pyQObjectType;
static PyTypeObject pyQObjectMethodType;


static void QObjectDealloc(pyQObject * self) {
	Py_XDECREF(self->_TWcontext);
	((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}
static void QObjectMethodDealloc(pyQObjectMethodObject * self) {
	Py_XDECREF(self->_TWcontext);
	Py_XDECREF(self->_methodName);
	((PyObject*)self)->ob_type->tp_free((PyObject*)self);
}

TWPythonPlugin::TWPythonPlugin()
{
	// Base class constructor
	QObject::QObject();
	
	// Initialize the python interpretor
	Py_Initialize();
}

TWPythonPlugin::~TWPythonPlugin()
{
	// Uninitialize the python interpreter
	Py_Finalize();
}

TWScript* TWPythonPlugin::newScript(const QString& fileName)
{
	return new PythonScript(this, fileName);
}

Q_EXPORT_PLUGIN2(TWPythonPlugin, TWPythonPlugin)


bool PythonScript::execute(TWInterface *tw) const
{
	PyObject * tmp;
	
	// Load the script
	QFile scriptFile(m_Filename);
	if (!scriptFile.open(QIODevice::ReadOnly)) {
		// handle error
		return false;
	}
	QTextStream stream(&scriptFile);
	QString contents = stream.readAll();
	scriptFile.close();

	// Create a separate sub-interpreter for this script
	PyThreadState* interpreter = Py_NewInterpreter();

	// Register the types
	if (!registerPythonTypes(tw->GetResult())) {
		Py_EndInterpreter(interpreter);
		return false;
	}
	
	pyQObject *TW;
	
	TW = (pyQObject*)QObjectToPython(tw);
	if (!TW) {
		tw->SetResult(tr("Could not create TW"));
		Py_EndInterpreter(interpreter);
		return false;
	}
	
	// Run the script
	PyObject * globals, * locals;
	globals = PyDict_New();
	locals = PyDict_New();
	
	// Create a dictionary of global variables
	// without the __builtins__ module, nothing would work!
	PyDict_SetItemString(globals, "__builtins__", PyEval_GetBuiltins());
	PyDict_SetItemString(globals, "TW", (PyObject*)TW);

	PyObject * ret = NULL;
	
	if (globals && locals)
		ret = PyRun_String(qPrintable(contents), Py_file_input, globals, locals);
	
	Py_XDECREF(globals);
	Py_XDECREF(locals);
	Py_XDECREF(ret);
	Py_XDECREF(TW);

	// Check for exceptions
	if (PyErr_Occurred()) {
		PyObject * errType, * errValue, * errTraceback;
		PyErr_Fetch(&errType, &errValue, &errTraceback);
		
		tmp = PyObject_Str(errValue);
		QString errString;
		if (!asQString(tmp, errString)) {
			Py_XDECREF(tmp);
			tw->SetResult(tr("Unknown error"));
			return false;
		}
		Py_XDECREF(tmp);
		tw->SetResult(errString);
		
		/////////////////////DEBUG
		// This prints the python error in the usual python way to stdout
		// Simply comment this block to prevent this behavior
		Py_XINCREF(errType);
		Py_XINCREF(errValue);
		Py_XINCREF(errTraceback);
		PyErr_Restore(errType, errValue, errTraceback);
		PyErr_Print();
		/////////////////////DEBUG
		
		Py_XDECREF(errType);
		Py_XDECREF(errValue);
		Py_XDECREF(errTraceback);

		Py_EndInterpreter(interpreter);
		return false;
	}

	// Finish
	Py_EndInterpreter(interpreter);
	return true;
}

bool PythonScript::registerPythonTypes(QVariant & errMsg) const
{
	// Register the Qobject wrapper
	pyQObjectType.tp_name = "QObject";
	pyQObjectType.tp_basicsize = sizeof(pyQObject);
	pyQObjectType.tp_dealloc = (destructor)QObjectDealloc;
	pyQObjectType.tp_flags = Py_TPFLAGS_DEFAULT;
	pyQObjectType.tp_doc = "QObject wrapper";
	pyQObjectType.tp_getattro = PythonScript::getAttribute;
	pyQObjectType.tp_setattro = PythonScript::setAttribute;
	
	if (PyType_Ready(&pyQObjectType) < 0) {
		errMsg = "Could not register QObject wrapper";
		return false;
	}

	// Register the TW method object
	pyQObjectMethodType.tp_name = "QObjectMethod";
	pyQObjectMethodType.tp_basicsize = sizeof(pyQObjectMethodObject);
	pyQObjectMethodType.tp_dealloc = (destructor)QObjectMethodDealloc;
	pyQObjectMethodType.tp_flags = Py_TPFLAGS_DEFAULT;
	pyQObjectMethodType.tp_doc = "QObject method wrapper";
	pyQObjectMethodType.tp_call = PythonScript::callMethod;
	
	
	if (PyType_Ready(&pyQObjectMethodType) < 0) {
		errMsg = "Could not register QObject method wrapper";
		return false;
	}
	return true;
}

/*static*/
PyObject * PythonScript::QObjectToPython(QObject * o)
{
	pyQObject * obj;
	obj = PyObject_New(pyQObject, &pyQObjectType);
	
	if (!obj) return NULL;
	
	obj = (pyQObject*)PyObject_Init((PyObject*)obj, &pyQObjectType);
	obj->_TWcontext = PyCObject_FromVoidPtr(o, NULL);
	return (PyObject*)obj;
}

/*static*/
PyObject* PythonScript::getAttribute(PyObject * o, PyObject * attr_name)
{
	QObject * obj;
	QMetaMethod method;
	QString propName;
	QVariant result;
	pyQObjectMethodObject * pyMethod;

	// Get the QObject* we operate on
	if (!PyObject_TypeCheck(o, &pyQObjectType)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("getattr: not a valid TW object")));
		return NULL;
	}
	if (!PyCObject_Check(((pyQObject*)o)->_TWcontext)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("getattr: not a valid TW object")));
		return NULL;
	}
	obj = (QObject*)PyCObject_AsVoidPtr((PyObject*)(((pyQObject*)o)->_TWcontext));
	
	if (!asQString(attr_name, propName)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("getattr: invalid property name")));
		return NULL;
	}
	
	switch (doGetProperty(obj, propName, result)) {
		case Property_DoesNotExist:
			PyErr_Format(PyExc_AttributeError, qPrintable(tr("getattr: object doesn't have property/method %s")), qPrintable(propName));
			return NULL;
		case Property_NotReadable:
			PyErr_Format(PyExc_AttributeError, qPrintable(tr("getattr: property %s is not readable")), qPrintable(propName));
			return NULL;
		case Property_Method:
			pyMethod = PyObject_New(pyQObjectMethodObject, &pyQObjectMethodType);
			pyMethod = (pyQObjectMethodObject*)PyObject_Init((PyObject*)pyMethod, &pyQObjectMethodType);
			Py_INCREF(pyMethod);
			pyMethod->_TWcontext = PyCObject_FromVoidPtr(obj, NULL);
			Py_XINCREF(attr_name);
			pyMethod->_methodName = (PyObject*)attr_name;
			return (PyObject*)pyMethod;
		case Property_OK:
			return PythonScript::VariantToPython(result);
		default:
			break;
	}
	// we should never reach this point
	return NULL;
}

/*static*/
int PythonScript::setAttribute(PyObject * o, PyObject * attr_name, PyObject * v)
{
	QObject * obj;
	QString propName;
	QMetaProperty prop;

	// Get the QObject* we operate on
	if (!PyObject_TypeCheck(o, &pyQObjectType)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("setattr: not a valid TW object")));
		return -1;
	}
	if (!PyCObject_Check(((pyQObject*)o)->_TWcontext)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("setattr: not a valid TW object")));
		return -1;
	}
	obj = (QObject*)PyCObject_AsVoidPtr((PyObject*)(((pyQObject*)o)->_TWcontext));

	// Get the parameters
	if (!asQString(attr_name, propName)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("setattr: invalid property name")));
		return -1;
	}

	switch (doSetProperty(obj, propName, PythonScript::PythonToVariant(v))) {
		case Property_DoesNotExist:
			PyErr_Format(PyExc_AttributeError, qPrintable(tr("setattr: object doesn't have property %s")), qPrintable(propName));
			return -1;
		case Property_NotWritable:
			PyErr_Format(PyExc_AttributeError, qPrintable(tr("setattr: property %s is not writable")), qPrintable(propName));
			return -1;
		case Property_OK:
			return 0;
		default:
			break;
	}
	// we should never reach this point
	return -1;
}

/*static*/
PyObject * PythonScript::callMethod(PyObject * o, PyObject * pyArgs, PyObject * kw)
{
	QObject * obj;
	QString methodName;
	QVariantList args;
	QVariant result;
	int i;
	
	// Get the QObject* we operate on
	obj = (QObject*)PyCObject_AsVoidPtr((PyObject*)(((pyQObjectMethodObject*)o)->_TWcontext));

	if (!asQString((PyObject*)(((pyQObjectMethodObject*)o)->_methodName), methodName)) {
		PyErr_SetString(PyExc_TypeError, qPrintable(tr("call: invalid method name")));
		return NULL;
	}
	
	for (i = 0; i < PyTuple_Size(pyArgs); ++i) {
		args.append(PythonScript::PythonToVariant(PyTuple_GetItem(pyArgs, i)));
	}
	
	switch (doCallMethod(obj, methodName, args, result)) {
		case Method_OK:
			return PythonScript::VariantToPython(result);
		case Method_DoesNotExist:
			PyErr_Format(PyExc_TypeError, qPrintable(tr("call: the method %s doesn't exist")), qPrintable(methodName));
			return NULL;
		case Method_WrongArgs:
			PyErr_Format(PyExc_TypeError, qPrintable(tr("call: couldn't call %s with the given arguments")), qPrintable(methodName));
			return NULL;
		case Method_Failed:
			PyErr_Format(PyExc_TypeError, qPrintable(tr("call: internal error while executing %s")), qPrintable(methodName));
			return NULL;
		default:
			break;
	}
	
	// we should never reach this point
	return NULL;
}


/*static*/
PyObject * PythonScript::VariantToPython(const QVariant & v)
{
	int i;
	QVariantList::const_iterator iList;
	QVariantList list;
#if QT_VERSION >= 0x040500
	QVariantHash::const_iterator iHash;
	QVariantHash hash;
#endif
	QVariantMap::const_iterator iMap;
	QVariantMap map;
	PyObject * pyList, * pyDict;

	if (v.isNull()) Py_RETURN_NONE;

	switch (v.type()) {
		case QVariant::Double:
			return Py_BuildValue("d", v.toDouble());
		case QVariant::Bool:
			if (v.toBool()) Py_RETURN_TRUE;
			else Py_RETURN_FALSE;
		case QVariant::Int:
			return Py_BuildValue("i", v.toInt());
		case QVariant::LongLong:
			return Py_BuildValue("L", v.toLongLong());
		case QVariant::UInt:
			return Py_BuildValue("I", v.toUInt());
		case QVariant::ULongLong:
			return Py_BuildValue("K", v.toULongLong());
		case QVariant::Char:
		case QVariant::String:
			return Py_BuildValue("s", qPrintable(v.toString()));
		case QVariant::List:
		case QVariant::StringList:
			list = v.toList();

			pyList = PyList_New(list.size());
			for (i = 0, iList = list.begin(); iList != list.end(); ++iList, ++i) {
				PyList_SetItem(pyList, i, PythonScript::VariantToPython(*iList));
			}
			return pyList;
#if QT_VERSION >= 0x040500
		case QVariant::Hash:
			hash = v.toHash();
			
			pyDict = PyDict_New();
			for (iHash = hash.begin(); iHash != hash.end(); ++iHash) {
				PyDict_SetItemString(pyDict, qPrintable(iHash.key()), PythonScript::VariantToPython(iHash.value()));
			}
			return pyDict;
#endif
		case QVariant::Map:
			map = v.toMap();
			
			pyDict = PyDict_New();
			for (iMap = map.begin(); iMap != map.end(); ++iMap) {
				PyDict_SetItemString(pyDict, qPrintable(iMap.key()), PythonScript::VariantToPython(iMap.value()));
			}
			return pyDict;
		case QMetaType::QObjectStar:
		case QMetaType::QWidgetStar:
			return PythonScript::QObjectToPython(v.value<QObject*>());
		default:
			PyErr_Format(PyExc_TypeError, qPrintable(tr("the type %s is currently not supported")), v.typeName());
			return NULL;
	}
	Py_RETURN_NONE;
}

/*static*/
QVariant PythonScript::PythonToVariant(PyObject * o)
{
	QVariantList list;
	QVariantMap map;
	PyObject * key, * value;
	Py_ssize_t i = 0;
	QString str;

	// in Python 3.x, the PyInt_* were removed in favor of PyLong_*
#if PY_MAJOR_VERSION < 3
	if (PyInt_Check(o)) return QVariant((int)PyInt_AsLong(o));
#endif
	if (PyBool_Check(o)) return QVariant((o == Py_True));
	if (PyLong_Check(o)) return QVariant((qlonglong)PyLong_AsLong(o));
	if (PyFloat_Check(o)) return QVariant(PyFloat_AsDouble(o));
	if (asQString(o, str)) return str;
	if (PyTuple_Check(o)) {
		for (i = 0; i < PyTuple_Size(o); ++i) {
			list.append(PythonToVariant(PyTuple_GetItem(o, i)));
		}
		return list;
	}
	if (PyList_Check(o)) {
		for (i = 0; i < PyList_Size(o); ++i) {
			list.append(PythonToVariant(PyList_GetItem(o, i)));
		}
		return list;
	}
	if (PyDict_Check(o)) {
		while (PyDict_Next(o, &i, &key, &value)) {
			map.insert(PythonScript::PythonToVariant(key).toString(), PythonScript::PythonToVariant(value));
		}
		return map;
	}
	// \TODO Complex numbers, byte arrays
	PyErr_Format(PyExc_TypeError, qPrintable(tr("the python type %s is currently not supported")), o->ob_type->tp_name);
	return QVariant();
}

/*static*/
bool PythonScript::asQString(PyObject * obj, QString & str)
{
	PyObject * tmp;

	// Get the parameters
	// In Python 3.x, the PyString_* were replaced by PyBytes_*
#if PY_MAJOR_VERSION < 3
	if (PyString_Check(obj)) {
		str = PyString_AsString(obj);
		return true;
	}
#else
	if (PyBytes_Check(obj)) {
		str = PyBytes_AsString(obj);
		return true;
	}
#endif
	if (PyUnicode_Check(obj)) {
		tmp = PyUnicode_AsUTF8String(obj);
#if PY_MAJOR_VERSION < 3
		str = QString::fromUtf8(PyString_AsString(tmp));
#else
		str = QString::fromUtf8(PyBytes_AsString(tmp));
#endif
		Py_XDECREF(tmp);
		return true;
	}
	return false;
}
