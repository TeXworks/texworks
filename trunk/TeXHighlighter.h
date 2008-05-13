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

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef TEX_HIGHLIGHTER_H
#define TEX_HIGHLIGHTER_H

#include <QSyntaxHighlighter>

#include <QTextCharFormat>

#include <hunspell/hunspell.h>

class QTextDocument;
class QTextCodec;

class TeXHighlighter : public QSyntaxHighlighter
{
	Q_OBJECT

public:
	TeXHighlighter(QTextDocument *parent = 0);
	
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
	
	bool isActive;

	Hunhandle *pHunspell;
	QTextCodec *spellingCodec;
};

#endif
