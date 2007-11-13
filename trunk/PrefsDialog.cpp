#include "PrefsDialog.h"
#include "QTeXApp.h"

#include <QSettings>
#include <QFontDatabase>
#include <QTextCodec>
#include <QSet>
#include <QFileDialog>
#include <QMainWindow>
#include <QToolBar>
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
	
	connect(binPathList, SIGNAL(itemSelectionChanged()), this, SLOT(updatePathButtons()));
	connect(pathUp, SIGNAL(clicked()), this, SLOT(movePathUp()));
	connect(pathDown, SIGNAL(clicked()), this, SLOT(movePathDown()));
	connect(pathAdd, SIGNAL(clicked()), this, SLOT(addPath()));
	connect(pathRemove, SIGNAL(clicked()), this, SLOT(removePath()));

	connect(toolList, SIGNAL(itemSelectionChanged()), this, SLOT(updateToolButtons()));
	connect(toolUp, SIGNAL(clicked()), this, SLOT(moveToolUp()));
	connect(toolDown, SIGNAL(clicked()), this, SLOT(moveToolDown()));
	connect(toolAdd, SIGNAL(clicked()), this, SLOT(addTool()));
	connect(toolRemove, SIGNAL(clicked()), this, SLOT(removeTool()));
	connect(toolEdit, SIGNAL(clicked()), this, SLOT(editTool()));
	
	connect(tabWidget, SIGNAL(currentChanged(int)), this, SLOT(changedTabPanel(int)));
}

void PrefsDialog::changedTabPanel(int index)
{
	// this all feels a bit hacky, but seems to keep things tidy on Mac OS X at least
	QWidget *page = tabWidget->widget(index);
	page->clearFocus();
	switch (index) {
		case 0: // General
			page->focusWidget()->clearFocus();
			break;
		case 1: // Editor
			editorFont->setFocus();
			editorFont->lineEdit()->selectAll();
			break;
		case 2: // Preview
			scale->setFocus();
			scale->selectAll();
			break;
		case 3: // Typesetting
			binPathList->setFocus();
			break;
	}
}

void PrefsDialog::updatePathButtons()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	pathRemove->setEnabled(selRow != -1);
	pathUp->setEnabled(selRow > 0);
	pathDown->setEnabled(selRow != -1 && selRow < binPathList->count() - 1);
}

void PrefsDialog::movePathUp()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = binPathList->takeItem(selRow);
		binPathList->insertItem(selRow - 1, item);
		binPathList->setCurrentItem(binPathList->item(selRow - 1));
	}
}

void PrefsDialog::movePathDown()
{
	int selRow = -1;
	if (binPathList->selectedItems().count() > 0)
		selRow = binPathList->currentRow();
	if (selRow != -1 &&  selRow < binPathList->count() - 1) {
		QListWidgetItem *item = binPathList->takeItem(selRow);
		binPathList->insertItem(selRow + 1, item);
		binPathList->setCurrentItem(binPathList->item(selRow + 1));
	}
}

void PrefsDialog::addPath()
{
	QString dir = QFileDialog::getExistingDirectory(this, tr("Choose Directory"),
					 "/usr", 0 /*QFileDialog::DontUseNativeDialog*/
								/*QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks*/);
	if (dir != "") {
		binPathList->addItem(dir);
		binPathList->setCurrentItem(binPathList->item(binPathList->count() - 1));
	}
}

void PrefsDialog::removePath()
{
	if (binPathList->currentRow() > -1)
		if (binPathList->currentItem()->isSelected())
			binPathList->takeItem(binPathList->currentRow());
}

void PrefsDialog::updateToolButtons()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	toolEdit->setEnabled(selRow != -1);
	toolRemove->setEnabled(selRow != -1);
	toolUp->setEnabled(selRow > 0);
	toolDown->setEnabled(selRow != -1 && selRow < toolList->count() - 1);
}

void PrefsDialog::moveToolUp()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = toolList->takeItem(selRow);
		toolList->insertItem(selRow - 1, item);
		toolList->setCurrentItem(toolList->item(selRow - 1));
		Engine e = engineList.takeAt(selRow);
		engineList.insert(selRow - 1, e);
	}
}

void PrefsDialog::moveToolDown()
{
	int selRow = -1;
	if (toolList->selectedItems().count() > 0)
		selRow = toolList->currentRow();
	if (selRow != -1 &&  selRow < toolList->count() - 1) {
		QListWidgetItem *item = toolList->takeItem(selRow);
		toolList->insertItem(selRow + 1, item);
		toolList->setCurrentItem(toolList->item(selRow + 1));
		Engine e = engineList.takeAt(selRow);
		engineList.insert(selRow + 1, e);
	}
}

void PrefsDialog::addTool()
{
	Engine e;
	e.setName(tr("New Tool"));
	e.setShowPdf(true);
	if (ToolConfig::doToolConfig(this, e) == QDialog::Accepted)
		engineList.append(e);
}

void PrefsDialog::removeTool()
{
	if (toolList->currentRow() > -1)
		if (toolList->currentItem()->isSelected()) {
			engineList.removeAt(toolList->currentRow());
			toolList->takeItem(toolList->currentRow());
		}
}

void PrefsDialog::editTool()
{
	if (toolList->currentRow() > -1)
		if (toolList->currentItem()->isSelected()) {
			Engine e = engineList[toolList->currentRow()];
			if (ToolConfig::doToolConfig(this, e) == QDialog::Accepted) {
				engineList[toolList->currentRow()] = e;
				toolList->currentItem()->setText(e.name());
			}
		}
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
			mediumIcons->setChecked(true);
			showText->setChecked(false);
			blankDocument->setChecked(true);
			break;

		case 1:
			// Editor
			{
				QFont font;
				editorFont->setEditText(font.family());
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
	PrefsDialog dlg(NULL);
	
	QSet<QTextCodec*> codecs;
	foreach (QByteArray codecName, QTextCodec::availableCodecs())
		codecs.insert(QTextCodec::codecForName(codecName));
	QStringList nameList;
	foreach (QTextCodec *codec, codecs)
		nameList.append(codec->name());
	nameList.sort();
	dlg.encoding->addItems(nameList);
	
	QSettings settings;
	// initialize controls based on the current settings
	
	// General
	int oldIconSize = settings.value("toolBarIconSize", 2).toInt();
	switch (oldIconSize) {
		case 1:
			dlg.smallIcons->setChecked(true);
			break;
		case 3:
			dlg.largeIcons->setChecked(true);
			break;
		default:
			dlg.mediumIcons->setChecked(true);
			break;
	}
	bool oldShowText = settings.value("toolBarShowText", false).toBool();
	dlg.showText->setChecked(oldShowText);
	
	// Editor
	dlg.syntaxColoring->setChecked(settings.value("syntaxColoring", true).toBool());
	dlg.wrapLines->setChecked(settings.value("wrapLines", true).toBool());
	dlg.tabWidth->setValue(settings.value("tabWidth", 32).toInt());
	QFontDatabase fdb;
	dlg.editorFont->addItems(fdb.families());
	QString fontString = settings.value("font").toString();
	QFont font;
	if (fontString != "")
		font.fromString(fontString);
	dlg.editorFont->setCurrentIndex(fdb.families().indexOf(font.family()));
	dlg.fontSize->setValue(font.pointSize());
	dlg.encoding->setCurrentIndex(nameList.indexOf("UTF-8"));

	// Preview
	
	// Typesetting
	QTeXApp *app = qobject_cast<QTeXApp*>(qApp);
	if (app != NULL) {
		dlg.binPathList->addItems(app->getBinaryPaths());
		dlg.engineList = app->getEngineList();
		foreach (Engine e, dlg.engineList) {
			dlg.toolList->addItem(e.name());
			dlg.defaultTool->addItem(e.name());
		}
	}
	dlg.autoHideOutput->setChecked(settings.value("autoHideConsole", true).toBool());
	if (dlg.binPathList->count() > 0)
		dlg.binPathList->setCurrentItem(dlg.binPathList->item(0));
	if (dlg.toolList->count() > 0)
		dlg.toolList->setCurrentItem(dlg.toolList->item(0));
	dlg.updatePathButtons();
	dlg.updateToolButtons();

	if (parent && parent->inherits("TeXDocument"))
		dlg.tabWidget->setCurrentIndex(1);
	else if (parent && parent->inherits("PDFDocument"))
		dlg.tabWidget->setCurrentIndex(2);
	
	dlg.show();

	DialogCode	result = (DialogCode)dlg.exec();

	if (result == Accepted) {
		// General
		int iconSize = 2;
		if (dlg.smallIcons->isChecked())
			iconSize = 1;
		else if (dlg.largeIcons->isChecked())
			iconSize = 3;
		bool showText = dlg.showText->isChecked();
		if (iconSize != oldIconSize || showText != oldShowText) {
			settings.setValue("toolBarIconSize", iconSize);
			settings.setValue("toolBarShowText", showText);
			foreach (QWidget *widget, qApp->topLevelWidgets()) {
				QMainWindow *theWindow = qobject_cast<QMainWindow*>(widget);
				if (theWindow != NULL)
					QTeXUtils::applyToolbarOptions(theWindow, iconSize, showText);
			}
		}
		
		// Editor
		settings.setValue("syntaxColoring", dlg.syntaxColoring->isChecked());
		settings.setValue("wrapLines", dlg.wrapLines->isChecked());
		settings.setValue("tabWidth", dlg.tabWidth->value());
		font = QFont(dlg.editorFont->currentText());
		font.setPointSize(dlg.fontSize->value());
		settings.setValue("font", font.toString());
		
		// Preview
		
		// Typesetting
		if (app != NULL) {
			QStringList paths;
			for (int i = 0; i < dlg.binPathList->count(); ++i)
				paths << dlg.binPathList->item(i)->text();
			app->setBinaryPaths(paths);
			app->setEngineList(dlg.engineList);
		}
	}

	return result;
}

ToolConfig::ToolConfig(QWidget *parent)
	: QDialog(parent)	
{
	init();
}
	
void ToolConfig::init()
{
	setupUi(this);
	
	connect(arguments, SIGNAL(itemSelectionChanged()), this, SLOT(updateArgButtons()));
	connect(argUp, SIGNAL(clicked()), this, SLOT(moveArgUp()));
	connect(argDown, SIGNAL(clicked()), this, SLOT(moveArgDown()));
	connect(argAdd, SIGNAL(clicked()), this, SLOT(addArg()));
	connect(argRemove, SIGNAL(clicked()), this, SLOT(removeArg()));
}

void ToolConfig::updateArgButtons()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	argRemove->setEnabled(selRow != -1);
	argUp->setEnabled(selRow > 0);
	argDown->setEnabled(selRow != -1 && selRow < arguments->count() - 1);
}

void ToolConfig::moveArgUp()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	if (selRow > 0) {
		QListWidgetItem *item = arguments->takeItem(selRow);
		arguments->insertItem(selRow - 1, item);
		arguments->setCurrentItem(arguments->item(selRow - 1));
	}
}

void ToolConfig::moveArgDown()
{
	int selRow = -1;
	if (arguments->selectedItems().count() > 0)
		selRow = arguments->currentRow();
	if (selRow != -1 &&  selRow < arguments->count() - 1) {
		QListWidgetItem *item = arguments->takeItem(selRow);
		arguments->insertItem(selRow + 1, item);
		arguments->setCurrentItem(arguments->item(selRow + 1));
	}
}

void ToolConfig::addArg()
{
	arguments->addItem(tr("NewArgument"));
	QListWidgetItem* item = arguments->item(arguments->count() - 1);
	item->setFlags(item->flags() | Qt::ItemIsEditable);
	arguments->setCurrentItem(item);
}

void ToolConfig::removeArg()
{
	if (arguments->currentRow() > -1)
		if (arguments->currentItem()->isSelected())
			arguments->takeItem(arguments->currentRow());
}

QDialog::DialogCode ToolConfig::doToolConfig(QWidget *parent, Engine &engine)
{
	ToolConfig dlg(parent);
	
	dlg.toolName->setText(engine.name());
	dlg.program->setText(engine.program());
	dlg.arguments->addItems(engine.arguments());
	for (int i = 0; i < dlg.arguments->count(); ++i) {
		QListWidgetItem *item = dlg.arguments->item(i);
		item->setFlags(item->flags() | Qt::ItemIsEditable);
	}
	dlg.viewPdf->setChecked(engine.showPdf());
	
	dlg.show();

	DialogCode	result = (DialogCode)dlg.exec();
	if (result == Accepted) {
		engine.setName(dlg.toolName->text());
		engine.setProgram(dlg.program->text());
		QStringList args;
		for (int i = 0; i < dlg.arguments->count(); ++i)
			args << dlg.arguments->item(i)->text();
		engine.setArguments(args);
		engine.setShowPdf(dlg.viewPdf->isChecked());
	}

	return result;
}
