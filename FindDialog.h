#ifndef FindDialog_H
#define FindDialog_H

#include <QDialog>

#include "ui_Find.h"
#include "ui_Replace.h"

class TeXDocument;

class FindDialog : public QDialog, private Ui::FindDialog
{
	Q_OBJECT

public:
	FindDialog(QWidget *parent);

	static void doFindDialog(TeXDocument *document);

private slots:
	void toggledRegexOption(bool checked);
	void toggledSelectionOption(bool checked);
	void checkRegex(const QString& str);

private:
	void init();
};

class ReplaceDialog : public QDialog, private Ui::ReplaceDialog
{
	Q_OBJECT

public:
	ReplaceDialog(QWidget *parent);
	
	static void doReplaceDialog(TeXDocument *document);

private slots:
	void toggledRegexOption(bool checked);
	void toggledSelectionOption(bool checked);
	void checkRegex(const QString& str);

private:
	void init();
};

#endif
