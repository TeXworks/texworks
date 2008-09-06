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

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the author,
	see <http://tug.org/texworks/>.
*/

#include "TWUtils.h"

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
#include <QTextCodec>
#include <QFile>
#include <QDirIterator>

#pragma mark === TWUtils ===

bool TWUtils::isPDFfile(const QString& fileName)
{
	QFile theFile(fileName);
	if (theFile.open(QIODevice::ReadOnly)) {
		QByteArray ba = theFile.peek(8);
		if (ba.startsWith("%PDF-1."))
			return true;
	}
	return false;
}

const QString TWUtils::getLibraryPath(const QString& subdir)
{
#ifdef Q_WS_MAC
	QString libPath(QDir::homePath() + "/Library/" + TEXWORKS_NAME + "/" + subdir);
#endif
#ifdef Q_WS_X11
	QString libPath;
	if (subdir == "dictionaries")
		libPath = "/usr/share/myspell/dicts";
	else
		libPath = QDir::homePath() + "/." + TEXWORKS_NAME + "/" + subdir;
#endif
#ifdef Q_WS_WIN
	QString libPath(QDir::homePath() + "/" + TEXWORKS_NAME + "/" + subdir);
#endif
	// check if libPath exists
	QFileInfo info(libPath);
	if (!info.exists()) {
		// create libPath
		if (QDir::root().mkpath(libPath)) {
			QString cwd = QDir::currentPath();
			if (QDir::setCurrent(libPath)) {
				// copy default contents from app resources into the library dir
				QDir resDir(":/resfiles/" + subdir);
				copyResources(resDir, libPath);
			}
			QDir::setCurrent(cwd);
		}
	}
	
	return libPath;
}

void TWUtils::copyResources(const QDir& resDir, const QString& libPath)
{
	QDirIterator iter(resDir, QDirIterator::Subdirectories);
	while (iter.hasNext()) {
		(void)iter.next();
		if (iter.fileInfo().isDir())
			continue;
		QString destPath = iter.fileInfo().canonicalPath();
		destPath.replace(resDir.path(), libPath);
		QFileInfo dest(destPath);
		if (!dest.exists())
			if (!QDir::root().mkpath(destPath))
				continue;
		if (QDir::setCurrent(destPath)) {
			QFile srcFile(iter.fileInfo().canonicalFilePath());
			srcFile.copy(iter.fileName());
			QDir::setCurrent(libPath);
		}
	}
}

QList<QTextCodec*> *TWUtils::codecList = NULL;

QList<QTextCodec*> *TWUtils::findCodecs()
{
	if (codecList != NULL)
		return codecList;

	codecList = new QList<QTextCodec*>;
	QMap<QString, QTextCodec*> codecMap;
	QRegExp iso8859RegExp("ISO[- ]8859-([0-9]+).*");
	foreach (int mib, QTextCodec::availableMibs()) {
		QTextCodec *codec = QTextCodec::codecForMib(mib);
		QString sortKey = codec->name().toUpper();
		int rank;
		if (sortKey.startsWith("UTF-8"))
			rank = 1;
		else if (sortKey.startsWith("UTF-16"))
			rank = 2;
		else if (iso8859RegExp.exactMatch(sortKey)) {
			if (iso8859RegExp.cap(1).size() == 1)
				rank = 3;
			else
				rank = 4;
		}
		else
			rank = 5;
		sortKey.prepend(QChar('0' + rank));
		codecMap.insert(sortKey, codec);
	}
	*codecList = codecMap.values();
	return codecList;
}

QStringList* TWUtils::translationList = NULL;

QStringList* TWUtils::getTranslationList()
{
	if (translationList != NULL)
		return translationList;

	translationList = new QStringList;
	QDir transDir(TWUtils::getLibraryPath("translations"));
	foreach (QFileInfo qmFileInfo, transDir.entryInfoList(QStringList(TEXWORKS_NAME "_*.qm"),
				QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase)) {
		QString locName = qmFileInfo.completeBaseName();
		locName.remove(TEXWORKS_NAME "_");
		*translationList << locName;
	}
	
	return translationList;
}

QStringList* TWUtils::dictionaryList = NULL;

QStringList* TWUtils::getDictionaryList()
{
	if (dictionaryList != NULL)
		return dictionaryList;

	dictionaryList = new QStringList;
	QDir dicDir(TWUtils::getLibraryPath("dictionaries"));
	foreach (QFileInfo affFileInfo, dicDir.entryInfoList(QStringList("*.aff"),
				QDir::Files | QDir::Readable, QDir::Name | QDir::IgnoreCase)) {
		QFileInfo dicFileInfo(affFileInfo.dir(), affFileInfo.completeBaseName() + ".dic");
		if (dicFileInfo.isReadable())
			*dictionaryList << dicFileInfo.completeBaseName();
	}
	
	return dictionaryList;
}

QHash<const QString,Hunhandle*> *TWUtils::dictionaries = NULL;

Hunhandle* TWUtils::getDictionary(const QString& language)
{
	if (language.isEmpty())
		return NULL;
	
	if (dictionaries == NULL)
		dictionaries = new QHash<const QString,Hunhandle*>;
	
	if (dictionaries->contains(language))
		return dictionaries->value(language);
	
	Hunhandle *h = NULL;
	const QString dictPath = getLibraryPath("dictionaries");
	QFileInfo affFile(dictPath + "/" + language + ".aff");
	QFileInfo dicFile(dictPath + "/" + language + ".dic");
	if (affFile.isReadable() && dicFile.isReadable()) {
		h = Hunspell_create(affFile.canonicalFilePath().toUtf8().data(),
							dicFile.canonicalFilePath().toUtf8().data());
		(*dictionaries)[language] = h;
	}
	return h;
}

QStringList* TWUtils::filters = NULL;
QStringList* TWUtils::filterList()
{
	 if (filters == NULL) {
		filters = new QStringList;
		QSettings settings;
		if (settings.contains("fileNameFilters"))
			*filters = settings.value("fileNameFilters").toStringList();
		else
			setDefaultFilters();
	 }
	 return filters;
}

void TWUtils::setDefaultFilters()
{
	filters->clear();
	*filters << QObject::tr("TeX documents (*.tex)");
	*filters << QObject::tr("LaTeX documents (*.ltx)");
	*filters << QObject::tr("BibTeX databases (*.bib)");
	*filters << QObject::tr("Style files (*.sty)");
	*filters << QObject::tr("Class files (*.cls)");
	*filters << QObject::tr("Documented macros (*.dtx)");
	*filters << QObject::tr("Auxiliary files (*.aux *.toc *.lot *.lof *.nav *.out *.snm *.ind *.idx *.bbl *.log)");
	*filters << QObject::tr("Text files (*.txt)");
	*filters << QObject::tr("PDF documents (*.pdf)");
#ifdef Q_WS_WIN
	*filters << QObject::tr("All files") + " (*.*)";	// unfortunately this doesn't work nicely on OS X or X11
#else
	*filters << QObject::tr("All files") + " (*)";
#endif
}

QString TWUtils::strippedName(const QString &fullFileName)
{
	return QFileInfo(fullFileName).fileName();
}

void TWUtils::updateRecentFileActions(QObject *parent, QList<QAction*> &actions, QMenu *menu) /* static */
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
		QString text = TWUtils::strippedName(files[i]);
		actions[i]->setText(text);
		actions[i]->setData(files[i]);
		actions[i]->setVisible(true);
	}
}

void TWUtils::updateWindowMenu(QWidget *window, QMenu *menu) /* static */
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

void TWUtils::ensureOnScreen(QWidget *window)
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

void TWUtils::zoomToScreen(QWidget *window)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect screenRect = desktop->availableGeometry(window);
	screenRect.setTop(screenRect.top() + window->geometry().y() - window->y());
	window->setGeometry(screenRect);
}

void TWUtils::zoomToHalfScreen(QWidget *window, bool rhs)
{
	QDesktopWidget *desktop = QApplication::desktop();
	QRect r = desktop->availableGeometry(window);
	int wDiff = window->frameGeometry().width() - window->width();
	int hDiff = window->frameGeometry().height() - window->height();
	if (rhs) {
		r.setLeft(r.left() + r.right() / 2);
		window->move(r.left(), r.top());
		window->resize(r.width() - wDiff, r.height() - hDiff);
	}
	else {
		r.setRight(r.left() + r.right() / 2 - 1);
		window->move(r.left(), r.top());
		window->resize(r.width() - wDiff, r.height() - hDiff);
	}
}

void TWUtils::sideBySide(QWidget *window1, QWidget *window2)
{
	zoomToHalfScreen(window1, false);
	zoomToHalfScreen(window2, true);
}

void TWUtils::tile(QList<QWidget*> windows)
{
}

void TWUtils::stack(QList<QWidget*> windows)
{
}

void TWUtils::applyToolbarOptions(QMainWindow *theWindow, int iconSize, bool showText)
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

bool TWUtils::findNextWord(const QString& text, int index, int& start, int& end)
{
	// try to do a sensible "word" selection for TeX documents, taking account of the form of control sequences:
	// given an index representing a caret,
	// if current char (following caret) is letter, apostrophe, or '@', extend in both directions
	//    include apostrophe if surrounded by letters
	//    include preceding backslash if any, unless word contains apostrophe
	// if preceding char is backslash, extend to include backslash only
	// if current char is number, extend in both directions
	// if current char is space or tab, extend in both directions to include all spaces or tabs
	// if current char is backslash, include next char; if letter or '@', extend to include all following letters or '@'
	// else select single char following index
	// returns TRUE if the resulting selection consists of word-forming chars

	start = end = index;

	if (text.length() < 1) // empty
		return false;

	if (index >= text.length()) // end of line
		return false;
	QChar	ch = text.at(index);

#define IS_WORD_FORMING(ch) (ch.isLetter() || ch.isMark())

	bool isControlSeq = false; // becomes true if we include an @ sign or a leading backslash
	bool includesApos = false; // becomes true if we include an apostrophe
	if (IS_WORD_FORMING(ch) || ch == '@' || ch == '\'' || ch == 0x2019) {
		if (ch == '@')
			isControlSeq = true;
		else if (ch == '\'' || ch == 0x2019)
			includesApos = true;
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (IS_WORD_FORMING(ch))
				continue;
			if (!includesApos && ch == '@') {
				isControlSeq = true;
				continue;
			}
			if (!isControlSeq && (ch == '\'' || ch == 0x2019) && start > 0 && IS_WORD_FORMING(text.at(start - 1))) {
				includesApos = true;
				continue;
			}
			++start;
			break;
		}
		if (start > 0 && text.at(start - 1) == '\\') {
			isControlSeq = true;
			--start;
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (IS_WORD_FORMING(ch))
				continue;
			if (!includesApos && ch == '@') {
				isControlSeq = true;
				continue;
			}
			if (!isControlSeq && (ch == '\'' || ch == 0x2019) && end < text.length() - 1 && IS_WORD_FORMING(text.at(end + 1))) {
				includesApos = true;
				continue;
			}
			break;
		}
		return !isControlSeq;
	}
	
	if (index > 0 && text.at(index - 1) == '\\') {
		start = index - 1;
		end = index + 1;
		return false;
	}
	
	if (ch.isNumber()) {
		// TODO: handle decimals, leading signs
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (ch.isNumber())
				continue;
			++start;
			break;
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (ch.isNumber())
				continue;
			break;
		}
		return false;
	}
	
	if (ch == ' ' || ch == '\t') {
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (!(ch == ' ' || ch == '\t')) {
				++start;
				break;
			}
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (!(ch == ' ' || ch == '\t'))
				break;
		}
		return false;
	}
	
	if (ch == '\\') {
		if (++end < text.length()) {
			ch = text.at(end);
			if (IS_WORD_FORMING(ch) || ch == '@')
				while (++end < text.length()) {
					ch = text.at(end);
					if (IS_WORD_FORMING(ch) || ch == '@')
						continue;
					break;
				}
			else
				++end;
		}
		return false;
	}

	// else the character is selected in isolation
	end = index + 1;
	return false;
}

QMap<QChar,QChar> TWUtils::pairOpeners;
QMap<QChar,QChar> TWUtils::pairClosers;

QChar TWUtils::closerMatching(QChar c)
{
	return pairClosers.value(c);
}

QChar TWUtils::openerMatching(QChar c)
{
	return pairOpeners.value(c);
}

void TWUtils::setUpPairs(const QList< QPair<QChar,QChar> >& pairs)
{
	pairOpeners.clear();
	pairClosers.clear();
	typedef QPair<QChar,QChar> charPairT; // otherwise foreach macro breaks
	foreach (charPairT p, pairs) {
		pairClosers[p.first] = p.second;
		pairOpeners[p.second] = p.first;
	}
}

QList< QPair<QChar,QChar> > TWUtils::defaultPairs()
{
	QList< QPair<QChar,QChar> > rval;
	rval.append(QPair<QChar,QChar>('(', ')'));
	rval.append(QPair<QChar,QChar>('[', ']'));
	rval.append(QPair<QChar,QChar>('{', '}'));
	rval.append(QPair<QChar,QChar>(0x00ab, 0x00bb));	// guillemots
	rval.append(QPair<QChar,QChar>(0x2018, 0x2019));	// single quotes
	rval.append(QPair<QChar,QChar>(0x201c, 0x201d));	// double quotes
	rval.append(QPair<QChar,QChar>(0x2039, 0x203a));	// single guillemots
	return rval;
}

int TWUtils::balanceDelim(const QString& text, int pos, QChar delim, int direction)
{
	int len = text.length();
	QChar c;
	while ((c = text[pos]) != delim) {
		if (openerMatching(c) != 0) {
			if (direction > 0)
				return -1;
			pos = balanceDelim(text, pos + direction, openerMatching(c), direction) + direction;
		}
		else if (closerMatching(c) != 0) {
			if (direction < 0)
				return -1;
			pos = balanceDelim(text, pos + direction, closerMatching(c), direction) + direction;
		}
		else
			pos += direction;
		if (pos < 0 || pos >= len)
			return -1;
	}
	return pos;
}

int TWUtils::findOpeningDelim(const QString& text, int pos)
	// find the first opening delimiter before offset /pos/
{
	while (--pos >= 0) {
		QChar c = text[pos];
		if (closerMatching(c) != 0)
			return pos;
	}
	return -1;
}

#pragma mark === SelWinAction ===

// action subclass used for dynamic window-selection items in the Window menu

SelWinAction::SelWinAction(QObject *parent, const QString &fileName)
	: QAction(parent)
{
	setText(TWUtils::strippedName(fileName));
	setData(fileName);
}

#pragma mark === CmdKeyFilter ===

// on OS X only, the singleton CmdKeyFilter object is attached to all TeXDocument editor widgets
// to stop Command-keys getting inserted into edit text items

CmdKeyFilter *CmdKeyFilter::filterObj = NULL;

CmdKeyFilter *CmdKeyFilter::filter()
{
	if (filterObj == NULL)
		filterObj = new CmdKeyFilter;
	return filterObj;
}

bool CmdKeyFilter::eventFilter(QObject *obj, QEvent *event)
{
#ifdef Q_WS_MAC
	if (event->type() == QEvent::KeyPress) {
		QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
		if ((keyEvent->modifiers() & Qt::ControlModifier) != 0) {
			if (keyEvent->key() <= 0x0ff
			 && keyEvent->key() != Qt::Key_Z
			 && keyEvent->key() != Qt::Key_X
			 && keyEvent->key() != Qt::Key_C
			 && keyEvent->key() != Qt::Key_V)
				return true;
		}
	}
#endif
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
