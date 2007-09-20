#include "FindDialog.h"
#include "TeXDocument.h"

#include <QSettings>

FindDialog::FindDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void FindDialog::init()
{
	setupUi(this);

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

	bool wrapOption = settings.value("searchWrap").toBool();
    checkBox_wrap->setChecked(wrapOption);

	bool selectionOption = settings.value("searchSelection").toBool();
    checkBox_selection->setChecked(selectionOption);
    checkBox_wrap->setEnabled(!selectionOption);
    checkBox_backwards->setEnabled(!selectionOption);

	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();
	checkBox_case->setChecked((flags & QTextDocument::FindCaseSensitively) != 0);
    checkBox_words->setChecked((flags & QTextDocument::FindWholeWords) != 0);
    checkBox_backwards->setChecked((flags & QTextDocument::FindBackward) != 0);
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
	checkBox_backwards->setEnabled(!checked);
	checkBox_wrap->setEnabled(!checked);
}

void FindDialog::checkRegex(const QString& str)
{
	if (checkBox_regex->isChecked()) {
		QRegExp regex(str);
		if (regex.isValid())
			regexStatus->setText("");
		else
			regexStatus->setText(tr("Invalid regular expression"));
	}
}

void FindDialog::doFindDialog(TeXDocument *document)
{
	FindDialog dlg(document);

	dlg.show();
	// doing this AFTER dlg.show() seems to be a necessary hack...
	dlg.setSizeGripEnabled(true);
	dlg.setSizeGripEnabled(false);	

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

		document->doFindAgain();
	}
}

ReplaceDialog::ReplaceDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void ReplaceDialog::init()
{
	setupUi(this);

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

	bool wrapOption = settings.value("searchWrap").toBool();
    checkBox_wrap->setChecked(wrapOption);

	bool selectionOption = settings.value("searchSelection").toBool();
    checkBox_selection->setChecked(selectionOption);
    checkBox_wrap->setEnabled(!selectionOption);
    checkBox_backwards->setEnabled(!selectionOption);

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
	checkBox_backwards->setEnabled(!checked);
	checkBox_wrap->setEnabled(!checked);
}

void ReplaceDialog::checkRegex(const QString& str)
{
	if (checkBox_regex->isChecked()) {
		QRegExp regex(str);
		if (regex.isValid())
			regexStatus->setText("");
		else
			regexStatus->setText(tr("Invalid regular expression"));
	}
}

void ReplaceDialog::doReplaceDialog(TeXDocument *document)
{
	ReplaceDialog dlg(document);

	dlg.show();
	// doing this AFTER dlg.show() seems to be a necessary hack...
	dlg.setSizeGripEnabled(true);
	dlg.setSizeGripEnabled(false);	

	DialogCode	result = (DialogCode)dlg.exec();

	/* FIXME: FINISH THIS */
}
