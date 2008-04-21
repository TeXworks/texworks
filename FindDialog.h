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

#ifndef FindDialog_H
#define FindDialog_H

#include <QDialog>

#include "ui_Find.h"
#include "ui_Replace.h"

class TeXDocument;
class QTextEdit;

class FindDialog : public QDialog, private Ui::FindDialog
{
	Q_OBJECT

public:
	FindDialog(QTextEdit *document);

	static DialogCode doFindDialog(QTextEdit *document);

private slots:
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
	void toggledRegexOption(bool checked);
	void toggledSelectionOption(bool checked);
	void checkRegex(const QString& str);
	void clickedReplace();
	void clickedReplaceAll();

private:
	void init(QTextEdit *document);
};

#endif
