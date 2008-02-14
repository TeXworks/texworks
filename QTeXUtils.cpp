#include "QTeXUtils.h"

#include "TeXDocument.h"
#include "PDFDocument.h"

#include <QFileDialog>
#include <QString>
#include <QMenu>
#include <QAction>
#include <QSettings>
#include <QStringList>
#include <QEvent>
#include <QKeyEvent>
#include <QDesktopWidget>
#include <QCompleter>

#pragma mark === QTeXUtils ===

QString QTeXUtils::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void QTeXUtils::updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu) /* static */
{
	QSettings settings;
	QStringList files = settings.value("recentFileList").toStringList();
	int numRecentFiles = files.size();

	while (actions.size() < numRecentFiles) {
		QAction *act = new QAction(parent);
		act->setVisible(false);
		QObject::connect(act, SIGNAL(triggered()), qApp, SLOT(openRecentFile()));
		actions.append(act);
		menu->addAction(act);
	}

	while (actions.size() > numRecentFiles) {
		QAction *act = actions.takeLast();
		delete act;
	}

	for (int i = 0; i < numRecentFiles; ++i) {
		QString text = QTeXUtils::strippedName(files[i]);
		actions[i]->setText(text);
		actions[i]->setData(files[i]);
		actions[i]->setVisible(true);
	}
}

void QTeXUtils::updateWindowMenu(QWidget *window, QMenu *menu) /* static */
{
	// shorten the menu by removing everything from the first "selectWindow" action onwards
	QList<QAction*> actions = menu->actions();
	for (QList<QAction*>::iterator i = actions.begin(); i != actions.end(); ++i) {
		SelWinAction *selWin = qobject_cast<SelWinAction*>(*i);
		if (selWin)
			menu->removeAction(*i);
	}
	while (!menu->actions().isEmpty() && menu->actions().last()->isSeparator())
		menu->removeAction(menu->actions().last());
	
	// append an item for each TeXDocument
	bool first = true;
	foreach (TeXDocument *texDoc, TeXDocument::documentList()) {
		if (first && !menu->actions().isEmpty())
			menu->addSeparator();
		first = false;
		SelWinAction *selWin = new SelWinAction(menu, texDoc->fileName());
		if (texDoc == qobject_cast<TeXDocument*>(window)) {
			selWin->setCheckable(true);
			selWin->setChecked(true);
		}
		QObject::connect(selWin, SIGNAL(triggered()), texDoc, SLOT(selectWindow()));
		menu->addAction(selWin);
	}
	
	// append an item for each PDFDocument
	first = true;
	foreach (PDFDocument *pdfDoc, PDFDocument::documentList()) {
		if (first && !menu->actions().isEmpty())
			menu->addSeparator();
		first = false;
		SelWinAction *selWin = new SelWinAction(menu, pdfDoc->fileName());
		if (pdfDoc == qobject_cast<PDFDocument*>(window)) {
			selWin->setCheckable(true);
			selWin->setChecked(true);
		}
		QObject::connect(selWin, SIGNAL(triggered()), pdfDoc, SLOT(selectWindow()));
		menu->addAction(selWin);
	}
}

void QTeXUtils::ensureOnScreen(QWidget *window)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(window);
	QRect adjustedFrame = window->frameGeometry();
	if (adjustedFrame.width() > screenRect.width())
		adjustedFrame.setWidth(screenRect.width());
	if (adjustedFrame.height() > screenRect.height())
		adjustedFrame.setHeight(screenRect.height());
	if (adjustedFrame.left() < screenRect.left())
		adjustedFrame.moveLeft(screenRect.left());
	else if (adjustedFrame.right() > screenRect.right())
		adjustedFrame.moveRight(screenRect.right());
	if (adjustedFrame.top() < screenRect.top())
		adjustedFrame.moveTop(screenRect.top());
	else if (adjustedFrame.bottom() > screenRect.bottom())
		adjustedFrame.moveBottom(screenRect.bottom());
	if (adjustedFrame != window->frameGeometry())
		window->setGeometry(adjustedFrame.adjusted(window->geometry().left() - window->frameGeometry().left(),
													window->geometry().top() - window->frameGeometry().top(),
													window->frameGeometry().right() - window->geometry().right(),
													window->frameGeometry().bottom() - window->geometry().bottom()
												));
}

void QTeXUtils::zoomToScreen(QWidget *window)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(window);
	screenRect.setTop(screenRect.top() + window->geometry().y() - window->y());
	window->setGeometry(screenRect);
}

void QTeXUtils::sideBySide(QWidget *window1, QWidget *window2)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(window1);
//	screenRect.setTop(screenRect.top() + window1->geometry().y() - window1->y());
	QRect r(screenRect);
	r.setRight(r.left() + r.right() / 2 - 1);
	//window1->setGeometry(r);
	int wDiff = window1->frameGeometry().width() - window1->width();
	int hDiff = window1->frameGeometry().height() - window1->height();
	window1->move(r.left(), r.top());
	window1->resize(r.width() - wDiff, r.height() - hDiff);
	r.setRight(screenRect.right());
	r.setLeft(r.left() + r.right() / 2);
	//window2->setGeometry(r);
	window2->move(r.left(), r.top());
	window2->resize(r.width() - wDiff, r.height() - hDiff);
}

void QTeXUtils::tile(QList<QWidget*> windows)
{
}

void QTeXUtils::stack(QList<QWidget*> windows)
{
}

void QTeXUtils::applyToolbarOptions(QMainWindow *theWindow, int iconSize, bool showText)
{
	iconSize = iconSize * 8 + 8;	// convert 1,2,3 to 16,24,32
	foreach (QObject *object, theWindow->children()) {
		QToolBar *theToolBar = qobject_cast<QToolBar*>(object);
		if (theToolBar != NULL) {
			theToolBar->setToolButtonStyle(showText ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonIconOnly);
			theToolBar->setIconSize(QSize(iconSize, iconSize));
		}
	}
}

#pragma mark === SelWinAction ===

// action subclass used for dynamic window-selection items in the Window menu

SelWinAction::SelWinAction(QObject *parent, const QString &fileName)
	: QAction(parent)
{
	setText(QTeXUtils::strippedName(fileName));
	setData(fileName);
}

#pragma mark === CmdKeyFilter ===

// the singleton CmdKeyFilter object is attached to all text-editing widgets

CmdKeyFilter *CmdKeyFilter::filterObj = NULL;

CmdKeyFilter *CmdKeyFilter::filter()
{
	if (filterObj == NULL)
		filterObj = new CmdKeyFilter;
	return filterObj;
}

bool CmdKeyFilter::eventFilter(QObject *obj, QEvent *event)
{
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if ((keyEvent->modifiers() & Qt::ControlModifier) != 0)
			if (keyEvent->key() != Qt::Key_Z
			 && keyEvent->key() != Qt::Key_X
			 && keyEvent->key() != Qt::Key_C
			 && keyEvent->key() != Qt::Key_V)
				return true;
	}
	return QObject::eventFilter(obj, event);
}

#pragma mark === CompletionFilter ===

// a CompletionFilter object is attached to all TeXDocument editors

bool CompletionFilter::eventFilter(QObject *obj, QEvent *event)
{
//	QTextEdit *te = qobject_cast<QTextEdit*>(obj);
//	if (te) {
		switch (event->type()) {
			case QEvent::KeyPress: {
				fprintf(stderr, "keypress\n");
//				te->keyPressEvent(static_cast<QKeyEvent*>(event));
				doc->keyPressEvent(static_cast<QKeyEvent*>(event));
				return QObject::eventFilter(obj, event);
			}
			
			case QEvent::FocusIn:
//				te->focusInEvent(static_cast<QFocusEvent*>(event));
				return QObject::eventFilter(obj, event);
				
			default:
				break;
		}
//	}

	return QObject::eventFilter(obj, event);
}

#pragma mark === Engine ===

Engine::Engine()
	: QObject()
{
}

Engine::Engine(const QString& name, const QString& program, const QStringList arguments, bool showPdf)
	: QObject(), f_name(name), f_program(program), f_arguments(arguments), f_showPdf(showPdf)
{
}

Engine::Engine(const Engine& orig)
	: QObject(), f_name(orig.f_name), f_program(orig.f_program), f_arguments(orig.f_arguments), f_showPdf(orig.f_showPdf)
{
}

Engine& Engine::operator=(const Engine& rhs)
{
	f_name = rhs.f_name;
	f_program = rhs.f_program;
	f_arguments = rhs.f_arguments;
	f_showPdf = rhs.f_showPdf;
	return *this;
}

const QString Engine::name() const
{
	return f_name;
}

const QString Engine::program() const
{
	return f_program;
}

const QStringList Engine::arguments() const
{
	return f_arguments;
}

bool Engine::showPdf() const
{
	return f_showPdf;
}

void Engine::setName(const QString& name)
{
	f_name = name;
}

void Engine::setProgram(const QString& program)
{
	f_program = program;
}

void Engine::setArguments(const QStringList& arguments)
{
	f_arguments = arguments;
}

void Engine::setShowPdf(bool showPdf)
{
	f_showPdf = showPdf;
}
