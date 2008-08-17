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

#include "FindDialog.h"
#include "TeXDocument.h"

#include <QSettings>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QTextBlock>

FindDialog::FindDialog(QTextEdit *parent)
	: QDialog(parent)
{
	init(parent);
}

void FindDialog::init(QTextEdit *document)
{
	setupUi(this);

	connect(checkBox_findAll, SIGNAL(toggled(bool)), this, SLOT(toggledFindAllOption(bool)));
	connect(checkBox_regex, SIGNAL(toggled(bool)), this, SLOT(toggledRegexOption(bool)));
	connect(checkBox_selection, SIGNAL(toggled(bool)), this, SLOT(toggledSelectionOption(bool)));
	connect(searchText, SIGNAL(textChanged(const QString&)), this, SLOT(checkRegex(const QString&)));

	QSettings settings;
	QString	str = settings.value("searchText").toString();
	searchText->setText(str);
	searchText->selectAll();

	bool regexOption = settings.value("searchRegex").toBool();
	checkBox_regex->setChecked(regexOption);
	checkBox_words->setEnabled(!regexOption);

	bool findAll = settings.value("searchFindAll").toBool();
	checkBox_findAll->setChecked(findAll);
	
	bool allFiles = settings.value("searchAllFiles").toBool();
	checkBox_allFiles->setChecked(allFiles);
	checkBox_allFiles->setEnabled(TeXDocument::documentList().count() > 1);

	bool selectionOption = settings.value("searchSelection").toBool();
	checkBox_selection->setEnabled(document->textCursor().hasSelection() && !findAll);
	checkBox_selection->setChecked(selectionOption && checkBox_selection->isEnabled());

	bool wrapOption = settings.value("searchWrap").toBool();
	checkBox_wrap->setEnabled(!(checkBox_selection->isEnabled() && checkBox_selection->isChecked()) && !findAll);
	checkBox_wrap->setChecked(wrapOption);

	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();
	checkBox_case->setChecked((flags & QTextDocument::FindCaseSensitively) != 0);
	checkBox_words->setChecked((flags & QTextDocument::FindWholeWords) != 0);
	checkBox_backwards->setChecked((flags & QTextDocument::FindBackward) != 0);
	checkBox_backwards->setEnabled(!findAll);
}

void FindDialog::toggledFindAllOption(bool checked)
{
	QTextEdit* document = qobject_cast<QTextEdit*>(parent());
	checkBox_selection->setEnabled(document != NULL && document->textCursor().hasSelection() && !checked);
	checkBox_wrap->setEnabled(!(checkBox_selection->isEnabled() && checkBox_selection->isChecked()) && !checked);
	checkBox_backwards->setEnabled(!checked);
}

void FindDialog::toggledRegexOption(bool checked)
{
	checkBox_words->setEnabled(!checked);
	if (checked)
		checkRegex(searchText->text());
	else
		regexStatus->setText("");
}

void FindDialog::toggledSelectionOption(bool checked)
{
	checkBox_wrap->setEnabled(!checked);
}

void FindDialog::checkRegex(const QString& str)
{
	if (checkBox_regex->isChecked()) {
		QRegExp regex(str);
		if (regex.isValid())
			regexStatus->setText("");
		else
			regexStatus->setText(tr("(invalid)"));
	}
}

QDialog::DialogCode FindDialog::doFindDialog(QTextEdit *document)
{
	FindDialog dlg(document);

	dlg.show();
	DialogCode	result = (DialogCode)dlg.exec();
	
	if (result == Accepted) {
		QSettings settings;
		QString str = dlg.searchText->text();
		settings.setValue("searchText", str);

		int flags = 0;
		if (dlg.checkBox_case->isChecked())
			flags |= QTextDocument::FindCaseSensitively;
		if (dlg.checkBox_words->isChecked())
			flags |= QTextDocument::FindWholeWords;
		if (dlg.checkBox_backwards->isChecked())
			flags |= QTextDocument::FindBackward;
		settings.setValue("searchFlags", (int)flags);

		settings.setValue("searchRegex", dlg.checkBox_regex->isChecked());
		settings.setValue("searchWrap", dlg.checkBox_wrap->isChecked());
		settings.setValue("searchSelection", dlg.checkBox_selection->isChecked());
		settings.setValue("searchFindAll", dlg.checkBox_findAll->isChecked());
		settings.setValue("searchAllFiles", dlg.checkBox_allFiles->isChecked());
	}

	return result;
}

ReplaceDialog::ReplaceDialog(QTextEdit *parent)
	: QDialog(parent)
{
	init(parent);
}

void ReplaceDialog::init(QTextEdit *document)
{
	setupUi(this);

	connect(checkBox_regex, SIGNAL(toggled(bool)), this, SLOT(toggledRegexOption(bool)));
	connect(checkBox_selection, SIGNAL(toggled(bool)), this, SLOT(toggledSelectionOption(bool)));
	connect(searchText, SIGNAL(textChanged(const QString&)), this, SLOT(checkRegex(const QString&)));

	buttonBox->button(QDialogButtonBox::Ok)->setText(tr("Replace"));
	connect(buttonBox->button(QDialogButtonBox::Ok), SIGNAL(clicked()), this, SLOT(clickedReplace()));
	buttonBox->button(QDialogButtonBox::SaveAll)->setText(tr("Replace All"));
	connect(buttonBox->button(QDialogButtonBox::SaveAll), SIGNAL(clicked()), this, SLOT(clickedReplaceAll()));
	buttonBox->button(QDialogButtonBox::Abort)->setText(tr("Cancel"));
	connect(buttonBox->button(QDialogButtonBox::Abort), SIGNAL(clicked()), this, SLOT(reject()));

	QSettings settings;
	QString	str = settings.value("searchText").toString();
	searchText->setText(str);
	searchText->selectAll();
	str = settings.value("replaceText").toString();
	replaceText->setText(str);

	bool regexOption = settings.value("searchRegex").toBool();
	checkBox_regex->setChecked(regexOption);
	checkBox_words->setEnabled(!regexOption);

	bool selectionOption = settings.value("searchSelection").toBool();
	checkBox_selection->setEnabled(document->textCursor().hasSelection());
	checkBox_selection->setChecked(selectionOption && checkBox_selection->isEnabled());

	bool wrapOption = settings.value("searchWrap").toBool();
	checkBox_wrap->setEnabled(!(checkBox_selection->isEnabled() && checkBox_selection->isChecked()));
	checkBox_wrap->setChecked(wrapOption);

	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();
	checkBox_case->setChecked((flags & QTextDocument::FindCaseSensitively) != 0);
	checkBox_words->setChecked((flags & QTextDocument::FindWholeWords) != 0);
	checkBox_backwards->setChecked((flags & QTextDocument::FindBackward) != 0);
}

void ReplaceDialog::toggledRegexOption(bool checked)
{
	checkBox_words->setEnabled(!checked);
	if (checked)
		checkRegex(searchText->text());
	else
		regexStatus->setText("");
}

void ReplaceDialog::toggledSelectionOption(bool checked)
{
	checkBox_wrap->setEnabled(!checked);
}

void ReplaceDialog::checkRegex(const QString& str)
{
	if (checkBox_regex->isChecked()) {
		QRegExp regex(str);
		if (regex.isValid())
			regexStatus->setText("");
		else
			regexStatus->setText(tr("(invalid)"));
	}
}

void ReplaceDialog::clickedReplace()
{
	done(1);
}

void ReplaceDialog::clickedReplaceAll()
{
	done(2);
}

ReplaceDialog::DialogCode ReplaceDialog::doReplaceDialog(QTextEdit *document)
{
	ReplaceDialog dlg(document);

	dlg.show();
	int	result = dlg.exec();

	if (result == 0)
		return Cancel;
	else {
		QSettings settings;
		QString str = dlg.searchText->text();
		settings.setValue("searchText", str);
		
		str = dlg.replaceText->text();
		settings.setValue("replaceText", str);

		int flags = 0;
		if (dlg.checkBox_case->isChecked())
			flags |= QTextDocument::FindCaseSensitively;
		if (dlg.checkBox_words->isChecked())
			flags |= QTextDocument::FindWholeWords;
		if (dlg.checkBox_backwards->isChecked())
			flags |= QTextDocument::FindBackward;
		settings.setValue("searchFlags", (int)flags);

		settings.setValue("searchRegex", dlg.checkBox_regex->isChecked());
		settings.setValue("searchWrap", dlg.checkBox_wrap->isChecked());
		settings.setValue("searchSelection", dlg.checkBox_selection->isChecked());

		return (result == 2) ? ReplaceAll : ReplaceOne;
	}
}


SearchResults::SearchResults(QWidget* parent)
	: QDockWidget(parent)
{
	setupUi(this);
	connect(table, SIGNAL(itemSelectionChanged()), this, SLOT(showSelectedEntry()));
}

void SearchResults::presentResults(const QList<SearchResult>& results, QMainWindow* parent, bool singleFile)
{
	SearchResults* resultsWindow = new SearchResults(parent);

	resultsWindow->table->setRowCount(results.count());
	int i = 0;
	foreach (const SearchResult &result, results) {
		resultsWindow->table->setItem(i, 0, new QTableWidgetItem(result.doc->fileName()));
		resultsWindow->table->setItem(i, 1, new QTableWidgetItem(QString::number(result.lineNo)));
		resultsWindow->table->setItem(i, 2, new QTableWidgetItem(QString::number(result.selStart)));
		resultsWindow->table->setItem(i, 3, new QTableWidgetItem(QString::number(result.selEnd)));
		resultsWindow->table->setItem(i, 4, new QTableWidgetItem(result.doc->getLineText(result.lineNo)));
		++i;
	}

	resultsWindow->table->setHorizontalHeaderLabels(QStringList() << tr("File") << tr("Line") << tr("Start") << tr("End") << tr("Text"));
	resultsWindow->table->horizontalHeader()->setResizeMode(4, QHeaderView::Stretch);
	resultsWindow->table->verticalHeader()->hide();
	resultsWindow->table->setColumnHidden(2, true);
	resultsWindow->table->setColumnHidden(3, true);

	resultsWindow->table->resizeColumnsToContents();
	resultsWindow->table->resizeRowsToContents();

	if (singleFile) {
		// remove any existing results dock from this parent window
		QList<SearchResults*> children = parent->findChildren<SearchResults*>();
		foreach (SearchResults* child, children) {
			parent->removeDockWidget(child);
			child->deleteLater();
		}
		resultsWindow->setAllowedAreas(Qt::TopDockWidgetArea|Qt::BottomDockWidgetArea);
		resultsWindow->setFloating(false);
		parent->addDockWidget(Qt::TopDockWidgetArea, resultsWindow);
	}
	else {
		resultsWindow->setAllowedAreas(Qt::NoDockWidgetArea);
		resultsWindow->setFloating(true);
		resultsWindow->setParent(NULL);
		resultsWindow->setWindowFlags(Qt::Window | Qt::WindowStaysOnTopHint);
	}
	
	resultsWindow->show();
}

void SearchResults::showSelectedEntry()
{
	int row = table->currentRow();
	QString fileName;
	int	lineNo = 1;
	QTableWidgetItem* item = table->item(row, 0);
	if (!item)
		return;
	fileName = item->text();
	item = table->item(row, 1);
	lineNo = item->text().toInt();
	item = table->item(row, 2);
	int selStart = item->text().toInt();
	item = table->item(row, 3);
	int selEnd = item->text().toInt();

	if (!fileName.isEmpty())
		TeXDocument::openDocument(fileName, lineNo, selStart, selEnd);
}
