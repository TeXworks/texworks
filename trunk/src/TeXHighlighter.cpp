/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2007-08  Jonathan Kew

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the author,
	see <http://tug.org/texworks/>.
*/

#include <QRegExp>
#include <QTextCodec>

#include "TeXHighlighter.h"
#include "TWUtils.h"

TeXHighlighter::TeXHighlighter(QTextDocument *parent)
	: QSyntaxHighlighter(parent)
	, isActive(true)
	, pHunspell(NULL)
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

	commentFormat.setForeground(Qt::red);
	rule.pattern = QRegExp("%.*");
	rule.format = commentFormat;
	highlightingRules.append(rule);
	
	spellFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	spellFormat.setUnderlineColor(Qt::red);
	spellCommentFormat = commentFormat;
	spellCommentFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	spellCommentFormat.setUnderlineColor(Qt::black);
}

void TeXHighlighter::highlightBlock(const QString &text)
{
	if (isActive) {
		int index;
		foreach (HighlightingRule rule, highlightingRules) {
			QRegExp expression(rule.pattern);
			index = text.indexOf(expression);
			while (index >= 0) {
				int length = expression.matchedLength();
				setFormat(index, length, rule.format);
				index = text.indexOf(expression, index + length);
			}
		}
	}

	if (pHunspell != NULL) {
		int index = 0;
		while (index < text.length()) {
			int start, end;
			if (TWUtils::findNextWord(text, index, start, end)) {
				QTextCharFormat currFormat = format(index);
				if (currFormat == controlSequenceFormat
					|| currFormat == environmentFormat
					|| currFormat == packageFormat) {
					// skip
				}
				else {
					QString word = text.mid(start, end - start);
					int spellResult = Hunspell_spell(pHunspell, spellingCodec->fromUnicode(word).data());
					if (spellResult == 0) {
						if (currFormat == commentFormat)
							setFormat(start, end - start, spellCommentFormat);
						else
							setFormat(start, end - start, spellFormat);
					}
				}
			}
			index = end;
		}
	}
}

void TeXHighlighter::setActive(bool active)
{
	isActive = active;
	rehighlight();
}

void TeXHighlighter::setSpellChecker(Hunhandle* h, QTextCodec* codec)
{
	pHunspell = h;
	spellingCodec = codec;
	rehighlight();
}
