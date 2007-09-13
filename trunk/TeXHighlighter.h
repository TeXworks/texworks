#ifndef TEX_HIGHLIGHTER_H
#define TEX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QHash>
#include <QTextCharFormat>

class QTextDocument;

class TeXHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	TeXHighlighter(QTextDocument *parent = 0);

protected:
	void highlightBlock(const QString &text);

private:
	struct HighlightingRule {
		QRegExp pattern;
		QTextCharFormat format;
	};
	QVector<HighlightingRule> highlightingRules;

	QTextCharFormat controlSequenceFormat;
	QTextCharFormat specialCharFormat;
	QTextCharFormat packageFormat;
	QTextCharFormat environmentFormat;
	QTextCharFormat commentFormat;
};

#endif
