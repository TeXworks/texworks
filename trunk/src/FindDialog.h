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

#ifndef FindDialog_H
#define FindDialog_H

#include <QDialog>
#include <QDockWidget>
#include <QList>

#include "ui_Find.h"
#include "ui_Replace.h"
#include "ui_SearchResults.h"

class TeXDocument;
class QTextEdit;

class FindDialog : public QDialog, private Ui::FindDialog
{
	Q_OBJECT

public:
	FindDialog(QTextEdit *document);

	static DialogCode doFindDialog(QTextEdit *document);

private slots:
	void toggledAllFilesOption(bool checked);
	void toggledFindAllOption(bool checked);
	void toggledRegexOption(bool checked);
	void toggledSelectionOption(bool checked);
	void checkRegex(const QString& str);

private:
	void init(QTextEdit *document);
};


class ReplaceDialog : public QDialog, private Ui::ReplaceDialog
{
	Q_OBJECT

public:
	ReplaceDialog(QTextEdit *parent);
	
	typedef enum {
		Cancel,
		ReplaceOne,
		ReplaceAll
	} DialogCode;
	
	static DialogCode doReplaceDialog(QTextEdit *document);

private slots:
	void toggledAllFilesOption(bool checked);
	void toggledRegexOption(bool checked);
	void toggledSelectionOption(bool checked);
	void checkRegex(const QString& str);
	void clickedReplace();
	void clickedReplaceAll();

private:
	void init(QTextEdit *document);
};


class SearchResult {
public:
	SearchResult(const TeXDocument* texdoc, int line, int start, int end)
		: doc(texdoc), lineNo(line), selStart(start), selEnd(end)
		{ }

	const TeXDocument* doc;
	int lineNo;
	int selStart;
	int selEnd;
};


class SearchResults	: public QDockWidget, private Ui::SearchResults
{
	Q_OBJECT
	
public:
	static void presentResults(const QList<SearchResult>& results, QMainWindow* parent, bool singleFile);
	
	SearchResults(QWidget* parent);
	
private slots:
	void showSelectedEntry();
};

#endif
