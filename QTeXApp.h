#include <QApplication>

class QTeXApp : public QApplication
{
	Q_OBJECT

public:
	QTeXApp(int argc, char *argv[]);

private slots:
	void about();

private:
	void init();
};
