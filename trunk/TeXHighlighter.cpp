#include <QRegExp>

#include "TeXHighlighter.h"

TeXHighlighter::TeXHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
{
	HighlightingRule rule;

	specialCharFormat.setForeground(Qt::darkRed);
	rule.pattern = QRegExp("[$#^_{}&]");
	rule.format = specialCharFormat;
	highlightingRules.append(rule);

	controlSequenceFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\\\(?:[A-Za-z@]+|.)");
	rule.format = controlSequenceFormat;
	highlightingRules.append(rule);

	environmentFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\\\\(?:begin|end)\\s*\\{[^}]*\\}");
	rule.format = environmentFormat;
	highlightingRules.append(rule);

	packageFormat.setForeground(Qt::darkBlue);
	rule.pattern = QRegExp("\\\\usepackage\\s*(?:\\[[^]]*\\]\\s*)?\\{[^}]*\\}");
	rule.format = packageFormat;
	highlightingRules.append(rule);

	commentFormat.setForeground(Qt::darkGray);
	rule.pattern = QRegExp("%.*");
	rule.format = commentFormat;
	highlightingRules.append(rule);
}

void TeXHighlighter::highlightBlock(const QString &text)
{
	foreach (HighlightingRule rule, highlightingRules) {
		QRegExp expression(rule.pattern);
		int index = text.indexOf(expression);
		while (index >= 0) {
			int length = expression.matchedLength();
			setFormat(index, length, rule.format);
			index = text.indexOf(expression, index + length);
		}
	}
}
