#ifndef QTeXUtils_H
#define QTeXUtils_H

#include <QAction>
#include <QString>
#include <QList>

#define TEXWORKS_NAME "TeXworks" /* app name, for use in menus, messages, etc */

class QMainWindow;
class QCompleter;
class TeXDocument;

// static utility methods
class QTeXUtils
{
public:
	// is the given file a PDF document?
	static bool isPDFfile(const QString& fileName);

	// return the path to our "library" folder for resources like templates, completion lists, etc
	static const QString getLibraryPath(const QString& subdir);

	// return a sorted list of all the available text codecs
	static QList<QTextCodec*> *findCodecs();

	// perform the updates to a menu; used by the documents to update their own menus
	static void updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu);

	// update the SelWinActions in a menu, used by the documents
	static void updateWindowMenu(QWidget *window, QMenu *menu);

	// return just the filename from a full pathname, suitable for UI display
	static QString strippedName(const QString &fullFileName);

	// window positioning utilities
	static void zoomToScreen(QWidget *window);
	static void sideBySide(QWidget *window1, QWidget *window2);
	static void tile(QList<QWidget*> windows);
	static void stack(QList<QWidget*> windows);
	static void ensureOnScreen(QWidget *window);
	static void applyToolbarOptions(QMainWindow *theWindow, int iconSize, bool showText);

private:
	QTeXUtils();

	static QList<QTextCodec*> *codecList;
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
