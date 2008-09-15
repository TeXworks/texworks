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
#include <QTextCursor>

#include "TeXHighlighter.h"
#include "TeXDocument.h"
#include "TWUtils.h"

#include <limits.h> // for INT_MAX

TeXHighlighter::TeXHighlighter(QTextDocument *parent, TeXDocument *texDocument)
	: QSyntaxHighlighter(parent)
	, texDoc(texDocument)
	, isActive(true)
	, isTagging(true)
	, pHunspell(NULL)
	, spellingCodec(NULL)
{
	HighlightingRule rule;

	specialCharFormat.setForeground(Qt::darkRed);
	rule.pattern = QRegExp("[$#^_{}&]");
	rule.format = specialCharFormat;
	highlightingRules.append(rule);

	environmentFormat.setForeground(Qt::darkGreen);
	rule.pattern = QRegExp("\\\\(?:begin|end)\\s*\\{[^}]*\\}");
	rule.format = environmentFormat;
	highlightingRules.append(rule);

	packageFormat.setForeground(Qt::darkBlue);
	rule.pattern = QRegExp("\\\\usepackage\\s*(?:\\[[^]]*\\]\\s*)?\\{[^}]*\\}");
	rule.format = packageFormat;
	highlightingRules.append(rule);

	controlSequenceFormat.setForeground(Qt::blue);
	rule.pattern = QRegExp("\\\\(?:[A-Za-z@]+|.)");
	rule.format = controlSequenceFormat;
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

	// default tag patterns for LaTeX (need to make this customizable)
	TagPattern patt;
	patt.pattern = QRegExp("^\\s*\\\\part\\s*(?:\\[[^]]*\\]\\s*)?\\{([^}]*)\\}");
	patt.level = 1;
	tagPatterns.append(patt);
	patt.pattern = QRegExp("^\\s*\\\\chapter\\s*(?:\\[[^]]*\\]\\s*)?\\{([^}]*)\\}");
	patt.level = 2;
	tagPatterns.append(patt);
	patt.pattern = QRegExp("^\\s*\\\\section\\s*(?:\\[[^]]*\\]\\s*)?\\{([^}]*)\\}");
	patt.level = 3;
	tagPatterns.append(patt);
	patt.pattern = QRegExp("^\\s*\\\\subsection\\s*(?:\\[[^]]*\\]\\s*)?\\{([^}]*)\\}");
	patt.level = 4;
	tagPatterns.append(patt);
	patt.pattern = QRegExp("^\\s*\\\\subsubsection\\s*(?:\\[[^]]*\\]\\s*)?\\{([^}]*)\\}");
	patt.level = 5;
	tagPatterns.append(patt);
	patt.pattern = QRegExp("^%:\\s*(.+)");
	patt.level = 0;
	tagPatterns.append(patt);
}

void TeXHighlighter::highlightBlock(const QString &text)
{
	if (isActive) {
		int index = 0;
		while (index < text.length()) {
			int firstIndex = INT_MAX, len;
			const HighlightingRule* firstRule = NULL;
			foreach (const HighlightingRule& rule, highlightingRules) {
				int foundIndex = text.indexOf(rule.pattern, index);
				if (foundIndex >= 0 && foundIndex < firstIndex) {
					firstIndex = foundIndex;
					firstRule = &rule;
				}
			}
			if (firstRule != NULL && (len = firstRule->pattern.matchedLength()) > 0) {
				setFormat(firstIndex, len, firstRule->format);
				index = firstIndex + len;
			}
			else
				break;
		}
	}

#if QT_VERSION >= 0x040400	/* the currentBlock() method is not available in 4.3.x */
	if (texDoc != NULL) {
		bool changed = false;
		if (texDoc->removeTags(currentBlock().position(), currentBlock().length()) > 0)
			changed = true;
		if (isTagging) {
			int index = 0;
			while (index < text.length()) {
				int firstIndex = INT_MAX, len;
				TagPattern* firstPatt = NULL;
				for (int i = 0; i < tagPatterns.count(); ++i) {
					TagPattern& patt = tagPatterns[i];
					int foundIndex = text.indexOf(patt.pattern, index);
					if (foundIndex >= 0 && foundIndex < firstIndex) {
						firstIndex = foundIndex;
						firstPatt = &patt;
					}
				}
				if (firstPatt != NULL && (len = firstPatt->pattern.matchedLength()) > 0) {
					QTextCursor	cursor(document());
					cursor.setPosition(currentBlock().position() + firstIndex);
					cursor.setPosition(currentBlock().position() + firstIndex + len, QTextCursor::KeepAnchor);
					QString text = firstPatt->pattern.cap(1);
					if (text.isEmpty())
						text = firstPatt->pattern.cap(0);
					texDoc->addTag(cursor, firstPatt->level, text);
					index = firstIndex + len;
					changed = true;
				}
				else
					break;
			}
		}
		if (changed)	
			texDoc->tagsChanged();
	}
#endif

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
