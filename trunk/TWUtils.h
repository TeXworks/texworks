/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-08  Jonathan Kew

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License along
	with this program; if not, write to the Free Software Foundation, Inc.,
	51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TWUtils_H
#define TWUtils_H

#include <QAction>
#include <QString>
#include <QList>
#include <QHash>
#include <QDir>

#include <hunspell/hunspell.h>

#define TEXWORKS_NAME "TeXworks" /* app name, for use in menus, messages, etc */

class QMainWindow;
class QCompleter;
class TeXDocument;

// static utility methods
class TWUtils
{
public:
	// is the given file a PDF document?
	static bool isPDFfile(const QString& fileName);

	// return the path to our "library" folder for resources like templates, completion lists, etc
	static const QString getLibraryPath(const QString& subdir);
	static void copyResources(const QDir& resDir, const QString& libPath);

	// return a sorted list of all the available text codecs
	static QList<QTextCodec*> *findCodecs();

	// get list of available translations
	static QStringList *getTranslationList();
	
	// get list of available dictionaries
	static QStringList *getDictionaryList();
	
	// get dictionary for a given language
	static Hunhandle *getDictionary(const QString& language);

	// perform the updates to a menu; used by the documents to update their own menus
	static void updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu);

	// update the SelWinActions in a menu, used by the documents
	static void updateWindowMenu(QWidget *window, QMenu *menu);

	// return just the filename from a full pathname, suitable for UI display
	static QString strippedName(const QString &fullFileName);

	// window positioning utilities
	static void zoomToScreen(QWidget *window);
	static void zoomToHalfScreen(QWidget *window, bool rhs = false);
	static void sideBySide(QWidget *window1, QWidget *window2);
	static void tile(QList<QWidget*> windows);
	static void stack(QList<QWidget*> windows);
	static void ensureOnScreen(QWidget *window);
	static void applyToolbarOptions(QMainWindow *theWindow, int iconSize, bool showText);

private:
	TWUtils();

	static QList<QTextCodec*>	*codecList;
	static QStringList			*dictionaryList;
	static QStringList			*translationList;

	static QHash<const QString,Hunhandle*>	*dictionaries;
};

// this special QAction class is used in Window menus, so that it's easy to recognize the dynamically-created items
class SelWinAction : public QAction
{
	Q_OBJECT
	
public:
	SelWinAction(QObject *parent, const QString &fileName);
};

// filter used to stop Command-keys getting inserted into edit text items
class CmdKeyFilter: public QObject
{
	Q_OBJECT

public:
	static CmdKeyFilter *filter();

protected:
	bool eventFilter(QObject *obj, QEvent *event);

private:
	static CmdKeyFilter *filterObj;
};

// specification of an "engine" used to process files
class Engine : public QObject
{
	Q_OBJECT
	
public:
	Engine();
	Engine(const QString& name, const QString& program, const QStringList arguments, bool showPdf);
	Engine(const Engine& orig);
	Engine& operator=(const Engine& rhs);

	const QString name() const;
	const QString program() const;
	const QStringList arguments() const;
	bool showPdf() const;

	void setName(const QString& name);
	void setProgram(const QString& program);
	void setArguments(const QStringList& arguments);
	void setShowPdf(bool showPdf);

private:
	QString f_name;
	QString f_program;
	QStringList f_arguments;
	bool f_showPdf;
};

#endif
