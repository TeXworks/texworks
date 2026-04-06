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
#ifndef Document_TextDocument_H
#define Document_TextDocument_H

#include "document/Document.h"

#include <QObject>
#include <QTextCursor>

#include <stdexcept>

class ScintillaDocument;
class CompletingEdit;
class QTextCodec;

namespace Tw {
namespace Document {

#define kLineEnd_Mask   0x00FF
#define kLineEnd_LF     0x0000
#define kLineEnd_CRLF   0x0001
#define kLineEnd_CR     0x0002

#define kLineEnd_Flags_Mask  0xFF00
#define kLineEnd_Mixed       0x0100

class FileIOException : public std::runtime_error
{
public:
	FileIOException(const QString & what) : std::runtime_error(what.toStdString()) { }
};

class UnsupportedEncodingException : public std::runtime_error
{
public:
	UnsupportedEncodingException(const QString & encoding) : std::runtime_error(encoding.toStdString()) { }
};

class TextDocument : public QObject, public Document
{
	Q_OBJECT
	friend class ::CompletingEdit;
	const int utf8MIB = 106;

#if defined(Q_OS_WIN)
	const unsigned int defaultLineEndings = kLineEnd_CRLF;
#else
	const unsigned int defaultLineEndings = kLineEnd_LF;
#endif

public:
	struct Tag {
		QTextCursor cursor;
		unsigned int level;
		QString text;
	};

	struct FileSettings {
		QTextCodec * codec{nullptr};
		unsigned int lineEnding{0};
		bool utf8BOM{false};
		bool ignoreUnsupportedEncoding{false};
	};

	explicit TextDocument(QObject * parent = nullptr);
	explicit TextDocument(const QString & text, QObject * parent = nullptr);

	const QList<Tag> & getTags() const { return _tags; }
	void addTag(const QTextCursor & cursor, const unsigned int level, const QString & text);
	unsigned int removeTags(int offset, int len);

	qsizetype length() const;
	bool isEmpty() const { return length() == 0; }

	QString line(const int line) const;
	int lineCount() const;

	virtual bool isModified() const;
	virtual void setModified(const bool modified = true);

	void saveFile(const QFileInfo & path);
	virtual void saveFile(const QFileInfo & path, const FileSettings & settings);

	void loadFile(const QFileInfo & path, QTextCodec * defaultCodec = nullptr);
	virtual void loadFile(const QFileInfo & path, FileSettings & settings, QTextCodec * defaultCodec = nullptr);

signals:
	void tagsChanged() const;
	void modificationChanged(bool modified) const;

private slots:
	void onModified(int position, int modification_type, const QByteArray &text, int length,
					int linesAdded, int line, int foldLevelNow, int foldLevelPrev);

protected:
	virtual void guessReadSettings(const QFileInfo & path, FileSettings & settings, const QByteArray & peekBytes);

	QList<Tag> _tags;
	// This is a non-owning pointer that doesn't get destroyed automatically;
	// set its parent to `this` (or another, appropriate QObject) to ensure
	// `m_scintilla` is destroyed at the right time
	ScintillaDocument * m_scintilla{nullptr};
	bool m_isModifiedCache{false};
};

} // namespace Document
} // namespace Tw

#endif // !defined(Document_TextDocument_H)
