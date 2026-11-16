/* Test-only TeXHighlighter stubs for TeXDocument. */
#include "TeXHighlighter.h"
#include "document/TeXDocument.h"

void NonblockingSyntaxHighlighter::setDocument(QTextDocument * document) { Q_UNUSED(document) }
void NonblockingSyntaxHighlighter::rehighlight() { }
void NonblockingSyntaxHighlighter::rehighlightBlock(const QTextBlock & block) { Q_UNUSED(block) }
void NonblockingSyntaxHighlighter::maybeRehighlightText(int position, int charsRemoved, int charsAdded)
{
	Q_UNUSED(position)
	Q_UNUSED(charsRemoved)
	Q_UNUSED(charsAdded)
}
void NonblockingSyntaxHighlighter::process() { }
void NonblockingSyntaxHighlighter::processWhenIdle() { }
TeXHighlighter::TeXHighlighter(Tw::Document::TeXDocument * parent) : NonblockingSyntaxHighlighter(parent) { }
void TeXHighlighter::highlightBlock(const QString & text) { Q_UNUSED(text) }
