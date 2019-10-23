#include <QtTest/QtTest>

class TestScripting : public QObject
{
	Q_OBJECT
private slots:
	void scriptLanguageName();
	void scriptLanguageURL();
	void canHandleFile();

	void isEnabled();
	void getScriptLanguagePlugin();
	void getFilename();

	void globals();

	void parseHeader_data();
	void parseHeader();

	void execute();
};
