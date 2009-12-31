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

#ifndef TWScript_H
#define TWScript_H

#include <QObject>
#include <QString>
#include <QFileInfo>
#include <QKeySequence>
#include <QStringList>
#include <QVariant>

/** \brief	Abstract base class for all Tw scripts
 *
 * \note This must be derived from QObject to enable interaction with e.g. menus
 */
class TWScript : public QObject
{
	Q_OBJECT
	
public:
	/** \brief	Types of scripts */
	enum ScriptType { 
		ScriptUnknown,		///< unknown script
		ScriptHook,			///< hook, i.e. a script that is called automatically when the execution reaches a certain point
		ScriptStandalone	///< standalone script, i.e. one that can be invoked by the user
	};
	/** \brief	Supported scripting languages */
	enum ScriptLanguage {
		LanguageQtScript,	///< QtScript (http://doc.trolltech.com/4.5/qtscript.html)
		LanguageLua,		///< Lua (http://www.lua.org/)
		LanguagePython		///< Python (http://www.python.org/)
	};
	
	/** \brief	Destructor
	 *
	 * Does nothing
	 */
	virtual ~TWScript() { }
	
	/** \brief	Get the type of the script
	 *
	 * \return	the script type
	 */
	ScriptType getType() const { return m_Type; }

	/** \brief	Get the filename of the script
	 *
	 * \return	the filename
	 */
	const QString& getFilename() const { return m_Filename; }

	/** \brief	Get the title of the script
	 *
	 * Used e.g. for adding a script to a menu
	 * \return	the title
	 */
	const QString& getTitle() const { return m_Title; }

	/** \brief	Get the description of the script
	 *
	 * \return	the description
	 */
	const QString& getDescription() const { return m_Description; }

	/** \brief	Get the author's name
	 *
	 * \return	the author's name
	 */
	const QString& getAuthor() const { return m_Author; }

	/** \brief	Get the script version
	 *
	 * \note	This is <i>not</i> the version <i>required to run</i> the script.
	 * \return	the script version
	 */
	const QString& getVersion() const { return m_Version; }

	/** \brief	Get the name of the hook this script should be connected to (if any)
	 *
	 * \return	the name of the hook, if this is a hook script. An empty string
	 * 			otherwise
	 */
	const QString& getHook() const { return m_Hook; }

	/** \brief	Get the name of the context where this script applies
	 *
	 * \return	The name of the window class where this script should be available
	 *			(TeXDocument or PDFDocument), or empty if the script is universal
	 */
	const QString& getContext() const { return m_Context; }
	
	/** \brief	Get the shortcut of this script
	 *
	 * \note	This is only useful for standalone scripts.
	 * \return	the shortcut
	 */
	const QKeySequence& getKeySequence() const { return m_KeySequence; }
	
	/** \brief	Set the file in which this script is stored
	 *
	 * \note	This method calls parseHeader() automatically if the specified
	 * 			file exists.
	 * \param	filename	the new filename
	 * \return	\c true on success, \c false otherwise
	 */
	bool setFile(const QString& filename);
	
	/** \brief Get the scripting language
	 *
	 * \note	This method must be implemented in derived classes
	 * \returns	the scripting language
	 */
	virtual ScriptLanguage getLanguage() const = 0;

	/** \brief Parse the script header
	 *
	 * \note	This method must be implemented in derived classes.
	 * \see	doParseHeader(QString, QString, QString, bool)
	 * \see	doParseHeader(QStringList)
	 * \return	\c true if successful, \c false if not (e.g. because the file
	 * 			is no valid Tw script)
	 */
	virtual bool parseHeader() = 0;

	/** \brief Run the script
	 *
	 * \param	context	the object from which the script was called; typically
	 * 					a TeXDocument or PDFDocument instance
	 * \param	result	variable to receive the result of the script execution;
	 * 					in the case of an error, this typically contains an
	 * 					error description
	 * \return	\c true on success, \c false if an error occured
	 */
	virtual bool run(QObject *context, QVariant& result) const = 0;
	
	/** \brief Check if two scripts are the same
	 *
	 * \note	This method compares the file paths
	 * \param	s	the script to compare to this one
	 * \return	\c true if *this == s, \c false otherwise
	 */
	bool operator==(const TWScript& s) const { return QFileInfo(m_Filename) == QFileInfo(s.m_Filename); }
	
protected:
	/** \brief	Constructor
	 *
	 * Does nothing
	 */
	TWScript() { }

	/** \brief	Constructor
	 *
	 * Sets the filename. Doesn't invoke setFile() or parseHeader().
	 */
	TWScript(const QString& filename) : m_Filename(filename) { }

	/** \brief	Clears all header data */
	void clearHeaderData();
	
	/** \brief	Convenience function to parse supported key:value pairs of the header
	 *
	 * Currently supported keys:
	 * - Title
	 * - Description
	 * - Author
	 * - Version
	 * - Script-Type
	 * - Hook
	 * - Shortcut
	 *
	 * \param	lines	the lines containing unparsed key:value pairs (but
	 * 					without any language-specific comment characters)
	 * \return	\c true if a title and type were found, \c false otherwise
	 */
	bool doParseHeader(const QStringList & lines);

	/** \brief	Convenience function to parse text-based script files
	 *
	 * Opens the text file specified by m_Filename, reads the first comment
	 * block and passes it on to doParseHeader(QStringList).
	 * \warning	You normally don't want to mix \a beginComment/\a endComment with
	 * 			\a Comment. In this case, the routine requires each line to be
	 * 			inside a comment block <em>and</em> start with \a Comment
	 * \param	beginComment	marker for the beginning of a comment block (e.g. /<!---->* in C++)
	 * \param	endComment		marker for the end of a comment block (e.g. *<!---->/ in C++)
	 * \param	Comment			marker for a one-line comment (e.g. /<!---->/ in C++)
	 * \param	skipEmpty		if \c true, empty lines are simply disregarded
	 * \return	\c true if a title and type were found, \c false otherwise
	 */
	bool doParseHeader(const QString& beginComment, const QString& endComment, const QString& Comment, bool skipEmpty = true);
	
	/** \brief	Possible results of calls to doGetProperty() and doSetProperty() */
	enum PropertyResult {
		Property_OK,			///< the get/set operation was successful
		Property_Method,		///< the get operation failed because the specified property is a method
		Property_DoesNotExist,	///< the specified property/method doesn't exist
		Property_NotReadable,	///< the get operation failed because the property is not readable
		Property_NotWritable,	///< the set operation failed because the property is not writable
		Property_Invalid		///< the get/set operation failed due to invalid data
	};

	/** \brief	Possible results of calls to doCallMethod() */
	enum MethodResult {
		Method_OK,				///< the call was successful
		Method_DoesNotExist,	///< the call failed because the specified method doesn't exist
		Method_WrongArgs,		///< the method exists but could not be called with the given arguments
		Method_Failed,			///< the method was called but the call failed
		Method_Invalid			///< the call failed due to invalid data
	};

	/** \brief	Get the value of the property of a QObject
	 *
	 * \note	This function relies on the meta object concept of Qt.
	 * \param	obj		pointer to the QObject the property value of which to get
	 * \param	name	the name of the property to get
	 * \param	value	variable to receive the value of the property on success
	 * \return	one of TWScript::PropertyResult
	 */
	static TWScript::PropertyResult doGetProperty(const QObject * obj, const QString& name, QVariant & value);

	/** \brief	Set the value of the property of a QObject
	 *
	 * \note	This function relies on the meta object concept of Qt.
	 * \param	obj		pointer to the QObject the property value of which to set
	 * \param	name	the name of the property to set
	 * \param	value	the new value of the property
	 * \return	one of TWScript::PropertyResult
	 */
	static TWScript::PropertyResult doSetProperty(QObject * obj, const QString& name, const QVariant & value);

	/** \brief	Call a method of a QObject
	 *
	 * \note	This function relies on the meta object concept of Qt.
	 * \param	obj		pointer to the QObject the method of which should be called
	 * \param	name	the name of the method to call
	 * \param	arguments	arguments to pass to the method
	 * \param	result	variable to receive the return value of the method on success
	 * \return	one of TWScript::MethodResult
	 */
	static TWScript::MethodResult doCallMethod(QObject * obj, const QString& name, QVariantList & arguments, QVariant & result);
	
	QString m_Filename;	///< the name of the file the script is stored in
	ScriptType m_Type;	///< the type of the script
	QString m_Title;	///< the title (e.g. for display in menus)
	QString m_Description;	///< the description
	QString m_Author;	///< the author's name
	QString m_Version;	///< the version
	QString m_Hook;		///< the hook this script implements (if any)
	QString m_Context;  ///< the main window class where this script can be used
	QKeySequence m_KeySequence;	///< the keyboard shortcut associated with this script
};

/** \brief	Interface all TW scripting plugins must implement */
class TWScriptPluginInterface
{
public:
	/** \brief	Constructor
	 *
	 * Does nothing
	 */
	TWScriptPluginInterface() { }
	
	/** \brief	Destructor
	 *
	 * Does nothing
	 */
	virtual ~TWScriptPluginInterface() { }
	
	/** \brief	Method to create a new script wrapper
	 *
	 * This method must be implemented in derived classes
	 * \return	the script wrapper, cast to TWScript
	 */
	virtual TWScript* newScript() = 0;
};

Q_DECLARE_INTERFACE(TWScript, "org.tug.texworks.Script/0.3")
Q_DECLARE_INTERFACE(TWScriptPluginInterface, "org.tug.texworks.ScriptPluginInterface/0.3")

#endif /* TWScript_H */
