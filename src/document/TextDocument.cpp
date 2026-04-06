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
#include <ScintillaTypes.h>

#include <QSaveFile>
#include <QTextCodec>

namespace Tw {
namespace Document {

// NB: the ScintillaDocument's parent is `this` so it gets automatically
// destroyed by Qt when this object is destroyed - without the need for an
// explicit destructor, std::unique_ptr, etc.
TextDocument::TextDocument(QObject * parent)
: QObject(parent)
, m_scintilla(new ScintillaDocument(this))
{
	connect(m_scintilla, &ScintillaDocument::modified, this, &TextDocument::onModified);
}

// NB: the ScintillaDocument's parent is `this` so it gets automatically
// destroyed by Qt when this object is destroyed - without the need for an
// explicit destructor, std::unique_ptr, etc.
TextDocument::TextDocument(const QString & text, QObject * parent)
: TextDocument(parent)
{
	m_scintilla->insert_string(0, text.toUtf8());
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

void TextDocument::saveFile(const QFileInfo &path)
{
	FileSettings defaultSettings{QTextCodec::codecForMib(utf8MIB), defaultLineEndings, false, true};
	saveFile(path, defaultSettings);
}

void TextDocument::saveFile(const QFileInfo &path, const FileSettings &settings)
{
	QSaveFile file(path.absoluteFilePath());
	file.setDirectWriteFallback(true);
	if (!file.open(QFile::WriteOnly)) {
		throw FileIOException(tr("Cannot write file \"%1\":\n%2").arg(path.absoluteFilePath(), file.errorString()));
	}
	if (settings.codec->mibEnum() == utf8MIB && settings.utf8BOM) {
		file.write("\xEF\xBB\xBF");
	}
	// FIXME: write in chunks to avoid holding the entire text in a temporary
	// QString; needs caution that we don't split a UTF-8 multi-byte character
	file.write(settings.codec->fromUnicode(text()));
	if (file.commit() == false) {
		throw FileIOException(tr("An error may have occurred while saving the file. "
								 "You might like to save a copy in a different location."));
	}
	m_scintilla->set_save_point();
}

void TextDocument::loadFile(const QFileInfo & path, QTextCodec * defaultCodec)
{
	FileSettings settings;
	loadFile(path, settings, defaultCodec);
}

void TextDocument::loadFile(const QFileInfo & path, FileSettings & settings, QTextCodec * defaultCodec)
{
	const int PEEK_LENGTH = 1024;

	QFile file(path.absoluteFilePath());
	// Not using QFile::Text because this prevents us reading "classic" Mac files
	// with CR-only line endings. See issue #242.
	if (!file.open(QFile::ReadOnly)) {
		throw FileIOException(tr("Cannot read file \"%1\":\n%2")
								 .arg(path.absoluteFilePath(), file.errorString()));
	}

	const QByteArray peekBytes(file.peek(PEEK_LENGTH));

	guessReadSettings(path, settings, peekBytes);

	if (settings.codec == nullptr) {
		if (defaultCodec != nullptr) {
			settings.codec = defaultCodec;
		}
		else {
			settings.codec = QTextCodec::codecForMib(utf8MIB);
		}
	}

	// When using the UTF-8 codec (mib = 106), byte order marks (BOMs) are
	// ignored during reading and not produced when writing. To keep them in
	// files that have them, we need to check for them ourselves.
	if (settings.codec->mibEnum() == 106 && peekBytes.size() >= 3 && peekBytes[0] == '\xEF' && peekBytes[1] == '\xBB' && peekBytes[2] == '\xBF') {
		settings.utf8BOM = true;
	}

	QString text = settings.codec->toUnicode(file.readAll());
	setText(text);
	m_scintilla->set_save_point();

	unsigned int numLineEndings{0};
	settings.lineEnding = 0;
	if (text.contains(QLatin1String("\r\n"))) {
		text.remove(QLatin1String("\r\n"));
		settings.lineEnding |= kLineEnd_CRLF;
		++numLineEndings;
	}
	if (text.contains(QChar::fromLatin1('\r'))) {
		text.remove(QChar::fromLatin1('\r'));
		settings.lineEnding |= kLineEnd_CR;
		++numLineEndings;
	}
	if (text.contains(QChar::fromLatin1('\n'))) {
		text.remove(QChar::fromLatin1('\n'));
		settings.lineEnding |= kLineEnd_LF;
		++numLineEndings;
	}

	if (numLineEndings == 0) {
		settings.lineEnding |= defaultLineEndings;
	}
}

void TextDocument::onModified(int position, int modification_type, const QByteArray &text, int length, int linesAdded, int line, int foldLevelNow, int foldLevelPrev)
{
	Q_UNUSED(position)
	Q_UNUSED(text)
	Q_UNUSED(length)
	Q_UNUSED(linesAdded)
	Q_UNUSED(line)
	Q_UNUSED(foldLevelNow)
	Q_UNUSED(foldLevelPrev)
	if (modification_type & static_cast<int>(Scintilla::ModificationFlags::InsertText) || modification_type & static_cast<int>(Scintilla::ModificationFlags::DeleteText)) {
		const bool modified{isModified()};
		if (modified != m_isModifiedCache) {
			m_isModifiedCache = modified;
			emit modificationChanged(modified);
		}
	}
}

void TextDocument::guessReadSettings(const QFileInfo &path, FileSettings &settings, const QByteArray &peekBytes)
{
	Q_UNUSED(path)
	Q_UNUSED(settings)
	Q_UNUSED(peekBytes)
}

qsizetype TextDocument::length() const
{
	if (!m_scintilla) {
		return 0;
	}
	return m_scintilla->length();
}

QString TextDocument::text() const
{
	return QString::fromUtf8(m_scintilla->get_char_range(0, m_scintilla->length()));
}

void TextDocument::setText(const QString & newText)
{
	// TODO: possibly only delete/insert the text if it has actually changed
	// to avoid side effects (such as messing with the undo-stack)
	m_scintilla->delete_chars(0, m_scintilla->length());
	m_scintilla->insert_string(0, newText.toUtf8());
}

} // namespace Document
} // namespace Tw
