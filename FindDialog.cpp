#include "FindDialog.h"
#include "TeXDocument.h"

#include <QSettings>
#include <QPushButton>

FindDialog::FindDialog(QTextEdit *parent)
	: QDialog(parent)
{
	init(parent);
}

void FindDialog::init(QTextEdit *document)
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
