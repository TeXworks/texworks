#ifndef COMPLETING_EDIT_H
#define COMPLETING_EDIT_H

#include <QTextEdit>

class QCompleter;
class QStandardItemModel;

class CompletingEdit : public QTextEdit
{
    Q_OBJECT

public:
    CompletingEdit(QWidget *parent = 0);
    ~CompletingEdit();

signals:
	void syncClick(int);

protected:
    virtual void keyPressEvent(QKeyEvent *e);
    virtual void focusInEvent(QFocusEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);

private slots:
	void clearCompleter();

private:
    void setCompleter(QCompleter *c);

	void showCompletion(const QString& completion, int insOffset = -1);
    void showCurrentCompletion();

	void loadCompletionsFromFile(QStandardItemModel *model, const QString& filename);
	void loadCompletionFiles(QCompleter *theCompleter);

    QCompleter *c;
	QTextCursor cmpCursor;

	QString prevCompletion; // used with multiple entries for the same key (e.g., "--")
	int itemIndex;
	int prevRow;

	static QCompleter	*sharedCompleter;
};

#endif // COMPLETING_EDIT_H
