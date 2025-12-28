/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019  Stefan Löffler

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <https://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <https://tug.org/texworks/>.
*/

#include "document/TextDocument.h"

#include <ScintillaDocument.h>

namespace Tw {
namespace Document {

// NB: the ScintillaDocument's parent is `this` so it gets automatically
// destroyed by Qt when this object is destroyed - without the need for an
// explicit destructor, std::unique_ptr, etc.
TextDocument::TextDocument(QObject * parent)
: QObject(parent)
, m_scintilla(new ScintillaDocument(this))
{
}

// NB: the ScintillaDocument's parent is `this` so it gets automatically
// destroyed by Qt when this object is destroyed - without the need for an
// explicit destructor, std::unique_ptr, etc.
TextDocument::TextDocument(const QString & text, QObject * parent)
: QObject(parent)
, m_scintilla(new ScintillaDocument(this))
{
	// FIXME: const-issues in ScintillaDocument [https://sourceforge.net/p/scintilla/bugs/2494/]
	QByteArray buffer{text.toUtf8()};
	m_scintilla->insert_string(0, buffer);
}

void TextDocument::addTag(const QTextCursor & cursor, const unsigned int level, const QString & text)
{
	QList<Tag>::iterator it;

	for (it = _tags.begin(); it != _tags.end(); ++it) {
		if (it->cursor.selectionStart() > cursor.selectionStart())
			break;
	}
	_tags.insert(it, {cursor, level, text});
	emit tagsChanged();
}

unsigned int TextDocument::removeTags(int offset, int len)
{
	unsigned int removed = 0;
	QList<Tag>::iterator start, end;

	for (start = _tags.begin(); start != _tags.end(); ++start) {
		if (start->cursor.selectionStart() >= offset)
			break;
	}
	for (end = start; end != _tags.end(); ++end) {
		if (end->cursor.selectionStart() < offset + len)
			++removed;
		else
			break;
	}
	if (removed > 0) {
		_tags.erase(start, end);
		emit tagsChanged();
	}
	return removed;
}

QString TextDocument::line(const int line) const
{
	if (!m_scintilla) {
		return {};
	}
	const int start = m_scintilla->line_start(line);
	const int end = m_scintilla->line_end(line);
	return QString::fromUtf8(m_scintilla->get_char_range(start, end - start));
}

int TextDocument::lineCount() const
{
	if (!m_scintilla) {
		return 0;
	}
	return m_scintilla->lines_total();
}

bool TextDocument::isModified() const
{
	if (!m_scintilla) {
		return false;
	}
	return m_scintilla->is_save_point() == false;
}

void TextDocument::setModified(const bool modified)
{
	// FIXME
}

qsizetype TextDocument::length() const
{
	if (!m_scintilla) {
		return 0;
	}
	return m_scintilla->length();
}

} // namespace Document
} // namespace Tw
