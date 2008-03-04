#ifndef PrefsDialog_H
#define PrefsDialog_H

#include <QDialog>
#include <QList>

#include "QTeXUtils.h"

#include "ui_PrefsDialog.h"
#include "ui_ToolConfig.h"

class PrefsDialog : public QDialog, private Ui::PrefsDialog
{
	Q_OBJECT

public:
	PrefsDialog(QWidget *parent);

	static DialogCode doPrefsDialog(QWidget *parent);

private slots:
	void buttonClicked(QAbstractButton *whichButton);
	
	void changedTabPanel(int index);

	void updatePathButtons();
	void movePathUp();
	void movePathDown();
	void addPath();
	void removePath();
	
	void updateToolButtons();
	void moveToolUp();
	void moveToolDown();
	void addTool();
	void removeTool();
	void editTool();

private:
	void init();
	void restoreDefaults();
	void refreshDefaultTool();
	void initPathAndToolLists();
	
	QList<Engine> engineList;
	
	bool pathsChanged;
	bool toolsChanged;
	
	static int sCurrentTab;
};

class ToolConfig : public QDialog, private Ui::ToolConfigDialog
{
	Q_OBJECT
	
public:
	ToolConfig(QWidget *parent);
	
	static DialogCode doToolConfig(QWidget *parent, Engine &engine);

private slots:
	void updateArgButtons();
	void moveArgUp();
	void moveArgDown();
	void addArg();
	void removeArg();

private:
	void init();
};

#endif
