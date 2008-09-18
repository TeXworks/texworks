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
	see <http://texworks.org/>.
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
	QDir configDir(TWUtils::getLibraryPath("configuration"));
	QRegExp whitespace("\\s+");

	QFile syntaxFile(configDir.filePath("syntax-patterns.txt"));
	if (syntaxFile.open(QIODevice::ReadOnly)) {
		while (1) {
			QByteArray ba = syntaxFile.readLine();
			if (ba.size() == 0)
				break;
			if (ba[0] == '#' || ba[0] == '\n')
				continue;
			QString line = QString::fromUtf8(ba.data(), ba.size());
			QStringList parts = line.split(whitespace, QString::SkipEmptyParts);
			if (parts.size() != 3)
				continue;
			QColor color(parts[0]);
			if (color.isValid()) {
				HighlightingRule rule;
				rule.format.setForeground(color);
				if (parts[1].compare("Y", Qt::CaseInsensitive) == 0) {
					rule.spellCheck = true;
					rule.spellFormat = rule.format;
					rule.spellFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
				}
				else
					rule.spellCheck = false;
				rule.pattern = QRegExp(parts[2]);
				if (rule.pattern.isValid() && !rule.pattern.isEmpty())
					highlightingRules.append(rule);
			}
		}
	}

	spellFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
	spellFormat.setUnderlineColor(Qt::red);

	// read tag-recognition patterns
	QFile tagPatternFile(configDir.filePath("tag-patterns.txt"));
	if (tagPatternFile.open(QIODevice::ReadOnly)) {
		while (1) {
			QByteArray ba = tagPatternFile.readLine();
			if (ba.size() == 0)
				break;
			if (ba[0] == '$' || ba[0] == '\n')
				continue;
			QString line = QString::fromUtf8(ba.data(), ba.size());
			QStringList parts = line.split(whitespace, QString::SkipEmptyParts);
			if (parts.size() != 2)
				continue;
			TagPattern patt;
			bool ok;
			patt.level = parts[0].toInt(&ok);
			if (ok) {
				patt.pattern = QRegExp(parts[1]);
				if (patt.pattern.isValid() && !patt.pattern.isEmpty())
					tagPatterns.append(patt);
			}
		}
	}
}

void TeXHighlighter::spellCheckRange(const QString &text, int index, int limit, const QTextCharFormat &spellFormat)
{
	while (index < limit) {
		int start, end;
		if (TWUtils::findNextWord(text, index, start, end)) {
			if (start < limit) {
				QString word = text.mid(start, end - start);
				int spellResult = Hunspell_spell(pHunspell, spellingCodec->fromUnicode(word).data());
				if (spellResult == 0)
					setFormat(start, end - start, spellFormat);
			}
		}
		index = end;
	}
}

void TeXHighlighter::highlightBlock(const QString &text)
{
	int index = 0;
	if (isActive) {
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
				if (pHunspell != NULL && firstIndex > index)
					spellCheckRange(text, index, firstIndex, spellFormat);
				setFormat(firstIndex, len, firstRule->format);
				index = firstIndex + len;
				if (pHunspell != NULL && firstRule->spellCheck)
					spellCheckRange(text, firstIndex, index, firstRule->spellFormat);
			}
			else
				break;
		}
	}
	if (pHunspell != NULL)
		spellCheckRange(text, index, text.length(), spellFormat);

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
