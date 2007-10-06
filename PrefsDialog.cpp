#include "PrefsDialog.h"

#include <QSettings>
#include <QTextCodec>
#include <QSet>
#include <QtAlgorithms>

PrefsDialog::PrefsDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void PrefsDialog::init()
{
	setupUi(this);
	
	connect(buttonBox, SIGNAL(clicked(QAbstractButton*)), this, SLOT(buttonClicked(QAbstractButton*)));
}

void PrefsDialog::buttonClicked(QAbstractButton *whichButton)
{
	if (buttonBox->buttonRole(whichButton) == QDialogButtonBox::ResetRole)
		restoreDefaults();
}

void PrefsDialog::restoreDefaults()
{
	switch (tabWidget->currentIndex()) {
		case 0:
			// General
			smallIcons->setChecked(true);
			showText->setChecked(false);
			break;

		case 1:
			// Editor
			{
				QFont font;
				editorFont->setCurrentFont(font);
				fontSize->setValue(font.pointSize());
			}
			tabWidth->setValue(36);
			wrapLines->setChecked(true);
			syntaxColoring->setChecked(true);
			encoding->setCurrentIndex(encoding->findText("UTF-8"));
			break;
	
		case 2:
			// Preview
			break;
		
		case 3:
			// Typesetting
			break;
	}
}

QDialog::DialogCode PrefsDialog::doPrefsDialog(QWidget *parent)
{
	PrefsDialog dlg(parent);
	
	QSet<QTextCodec*> codecs;
	foreach (QByteArray codecName, QTextCodec::availableCodecs())
		codecs.insert(QTextCodec::codecForName(codecName));
	QStringList nameList;
	foreach (QTextCodec *codec, codecs)
		nameList.append(codec->name());
	nameList.sort();
	dlg.encoding->addItems(nameList);
	dlg.encoding->setCurrentIndex(nameList.indexOf("UTF-8"));
	
	if (parent && parent->inherits("TeXDocument"))
		dlg.tabWidget->setCurrentIndex(1);
	else if (parent && parent->inherits("PDFDocument"))
		dlg.tabWidget->setCurrentIndex(2);
	
	QSettings settings;
	// initialize controls based on the current settings
	
	// General
	
	// Editor
	dlg.syntaxColoring->setChecked(settings.value("syntaxColoring", true).toBool());
	dlg.wrapLines->setChecked(settings.value("wrapLines", true).toBool());
	dlg.tabWidth->setValue(settings.value("tabWidth", 32).toInt());
	QString fontString = settings.value("font").toString();
	QFont font;
	if (fontString != "")
		font.fromString(fontString);
	dlg.editorFont->setCurrentFont(font);
	dlg.fontSize->setValue(font.pointSize());

	dlg.show();
	// doing this AFTER dlg.show() seems to be a necessary hack...
	dlg.setSizeGripEnabled(true);
	dlg.setSizeGripEnabled(false);	

	DialogCode	result = (DialogCode)dlg.exec();
	if (result == Accepted) {
		// General
		
		// Editor
		settings.setValue("syntaxColoring", dlg.syntaxColoring->isChecked());
		settings.setValue("wrapLines", dlg.wrapLines->isChecked());
		settings.setValue("tabWidth", dlg.tabWidth->value());
		font = dlg.editorFont->currentFont();
		font.setPointSize(dlg.fontSize->value());
		settings.setValue("font", font.toString());
		
		// Preview
		
		// Typesetting

	}

	return result;
}
