#include "FindDialog.h"
#include "TeXDocument.h"

FindDialog::FindDialog(QWidget *parent)
	: QDialog(parent)
{
	init();
}

void
FindDialog::init()
{
	setupUi(this);
}

void FindDialog::doFindDialog(TeXDocument *document)
{
	FindDialog dlg(document);
	dlg.show();
	dlg.exec();
}
