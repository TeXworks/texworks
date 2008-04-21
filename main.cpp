#include "TWApp.h"
#include "TeXDocument.h"

int main(int argc, char *argv[])
{
	TWApp app(argc, argv);

	// first argument is the executable name, so we skip that
	for (int i = 1; i < argc; ++i)
		app.open(argv[i]);
	
	if (TeXDocument::documentList().size() == 0) {
		TeXDocument *mainWin = new TeXDocument;
		mainWin->show();
	}

	return app.exec();
}
