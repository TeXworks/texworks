/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/

#include "languageservices/LanguageServiceNavigation.h"

#include <QTextBlock>
#include <QTextDocument>

namespace Tw {
namespace LanguageServices {

bool cursorForLanguageRange(QTextDocument * document, const LanguageRange & range, QTextCursor & cursor)
{
	if (!document || range.start.line < 0 || range.start.character < 0
	    || range.end.line < range.start.line || range.end.character < 0)
		return false;
	const QTextBlock startBlock = document->findBlockByNumber(range.start.line);
	const QTextBlock endBlock = document->findBlockByNumber(range.end.line);
	if (!startBlock.isValid() || !endBlock.isValid()
	    || range.start.character > startBlock.text().size()
	    || range.end.character > endBlock.text().size())
		return false;
	const int start = startBlock.position() + range.start.character;
	const int end = endBlock.position() + range.end.character;
	if (end < start)
		return false;
	cursor = QTextCursor(document);
	cursor.setPosition(start);
	cursor.setPosition(end, QTextCursor::KeepAnchor);
	return true;
}

} // namespace LanguageServices
} // namespace Tw
