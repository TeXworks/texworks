#ifndef FindDialog_H
#define FindDialog_H

#include <QDialog>

#include "ui_Find.h"

class TeXDocument;

class FindDialog : public QDialog, private Ui::FindDialog
{
	Q_OBJECT

public:
	FindDialog(QWidget *parent);

	static void doFindDialog(TeXDocument *document);

protected:

private slots:
	void toggledRegexOption(bool checked);
	void checkRegex(const QString& str);

private:
	void init();
};

#endif
