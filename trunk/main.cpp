#include <QApplication>

#include "TeXDocument.h"
#include "PDFDocument.h"
#include "QTeXApp.h"

int main(int argc, char *argv[])
{
	QTeXApp app(argc, argv);

	TeXDocument mainWin;
	mainWin.show();

	PDFDocument pdfWin("/Volumes/Nenya/texlive/Master/texmf-dist/doc/xelatex/fontspec/fontspec.pdf");
	pdfWin.show();

	return app.exec();
}

