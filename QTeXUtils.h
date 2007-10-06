#ifndef QTeXUtils_H
#define QTeXUtils_H

#include <QAction>
#include <QString>
#include <QList>

// static utility methods
class QTeXUtils
{
public:
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

private:
	QTeXUtils();
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

#endif
