#ifndef PrefsDialog_H
#define PrefsDialog_H

#include <QDialog>

#include "ui_PrefsDialog.h"

class PrefsDialog : public QDialog, private Ui::PrefsDialog
{
	Q_OBJECT

public:
	PrefsDialog(QWidget *parent);

	static DialogCode doPrefsDialog(QWidget *parent);

private slots:
	void buttonClicked(QAbstractButton *whichButton);

private:
	void init();
	void restoreDefaults();
};

#endif
