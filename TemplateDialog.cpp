#include "TemplateDialog.h"
#include "TeXHighlighter.h"
#include "TWUtils.h"

#include <QDirModel>
#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QSettings>

TemplateDialog::TemplateDialog(QWidget *parent)
	: QDialog(parent)
	, model(NULL)
{
	init();
}

TemplateDialog::~TemplateDialog()
{
	if (model != NULL)
		delete model;
}

void TemplateDialog::init()
{
	setupUi(this);

	QString templatePath = TWUtils::getLibraryPath("templates");
		// do this before creating the model, as getLibraryPath might initialize a new dir
		
	model = new QDirModel;
	treeView->setModel(model);
	treeView->setRootIndex(model->index(templatePath));
	treeView->expandAll();
	treeView->resizeColumnToContents(0);
	treeView->hideColumn(2);
	
	connect(treeView->selectionModel(), SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
			this, SLOT(selectionChanged(const QItemSelection&, const QItemSelection&)));

	QSettings settings;
	if (settings.value("wrapLines", true).toBool()) {
		new TeXHighlighter(textEdit->document());
	}
}

void TemplateDialog::selectionChanged(const QItemSelection &selected, const QItemSelection &/*deselected*/)
{
	textEdit->clear();
	if (selected.indexes().count() > 0) {
		QString filePath(model->filePath(selected.indexes()[0]));
		QFileInfo fileInfo(filePath);
		if (fileInfo.isFile() && fileInfo.isReadable()) {
			QFile file(filePath);
			if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
				QTextStream in(&file);
				textEdit->setPlainText(in.readAll());
			}
		}
	}
}

QString TemplateDialog::doTemplateDialog(QWidget *parent)
{
	QString rval;

	TemplateDialog dlg(NULL);
	dlg.show();
	DialogCode	result = (DialogCode)dlg.exec();

	if (result == Accepted) {
		QModelIndexList selection = dlg.treeView->selectionModel()->selectedRows();
		if (selection.count() > 0) {
			QString filePath(dlg.model->filePath(selection[0]));
			QFileInfo fileInfo(filePath);
			if (fileInfo.isFile() && fileInfo.isReadable())
				rval = filePath;
		}
	}
	
	return rval;
}
