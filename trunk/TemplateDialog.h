#ifndef TemplateDialog_H
#define TemplateDialog_H

#include <QDialog>
#include <QString>
#include <QItemSelection>

#include "QTeXUtils.h"

#include "ui_TemplateDialog.h"

class QDirModel;

class TemplateDialog : public QDialog, private Ui::TemplateDialog
{
	Q_OBJECT

public:
	TemplateDialog(QWidget *parent);
	virtual ~TemplateDialog();

	static QString doTemplateDialog(QWidget *parent);

private slots:
	void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	
private:
	void init();

	QDirModel *model;
};

#endif
