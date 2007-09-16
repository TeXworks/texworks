#include "FindDialog.h"
#include "TeXDocument.h"

#include <QSettings>

FindDialog::FindDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void
FindDialog::init()
{
	setupUi(this);

	connect(checkBox_regex, SIGNAL(toggled(bool)), this, SLOT(toggledRegexOption(bool)));
	connect(checkBox_selection, SIGNAL(toggled(bool)), this, SLOT(toggledSelectionOption(bool)));
	connect(searchText, SIGNAL(textChanged(const QString&)), this, SLOT(checkRegex(const QString&)));
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

	QSettings settings;
	QString	searchText = settings.value("searchText").toString();
	dlg.searchText->setText(searchText);
	dlg.searchText->selectAll();

	bool regexOption = settings.value("searchRegex").toBool();
    dlg.checkBox_regex->setChecked(regexOption);
    dlg.checkBox_words->setEnabled(!regexOption);

	bool wrapOption = settings.value("searchWrap").toBool();
    dlg.checkBox_wrap->setChecked(wrapOption);

	bool selectionOption = settings.value("searchSelection").toBool();
    dlg.checkBox_selection->setChecked(selectionOption);
    dlg.checkBox_wrap->setEnabled(!selectionOption);
    dlg.checkBox_backwards->setEnabled(!selectionOption);

	QTextDocument::FindFlags flags = (QTextDocument::FindFlags)settings.value("searchFlags").toInt();
	dlg.checkBox_case->setChecked((flags & QTextDocument::FindCaseSensitively) != 0);
    dlg.checkBox_words->setChecked((flags & QTextDocument::FindWholeWords) != 0);
    dlg.checkBox_backwards->setChecked((flags & QTextDocument::FindBackward) != 0);

	dlg.show();
	DialogCode	result = (DialogCode)dlg.exec();
	
	if (result == Accepted) {
		searchText = dlg.searchText->text();
		settings.setValue("searchText", searchText);

		flags = 0;
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
