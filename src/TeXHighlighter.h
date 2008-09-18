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

#ifndef TEX_HIGHLIGHTER_H
#define TEX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QTextCharFormat>

#include <hunspell.h>

class QTextDocument;
class QTextCodec;
class TeXDocument;

class TeXHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	TeXHighlighter(QTextDocument *parent, TeXDocument *texDocument = NULL);
	
	void setActive(bool active);

	void setSpellChecker(Hunhandle *h, QTextCodec *codec);

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

	QTextCharFormat spellFormat;
	QTextCharFormat spellCommentFormat;

	struct TagPattern {
		QRegExp pattern;
		unsigned int level;
	};
	QVector<TagPattern> tagPatterns;
	
	TeXDocument	*texDoc;

	bool isActive;
	bool isTagging;

	Hunhandle	*pHunspell;
	QTextCodec	*spellingCodec;
};

#endif
