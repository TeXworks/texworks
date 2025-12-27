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

#include <memory>
#include <QObject>
#include <QTextCursor>

class ScintillaDocument;
class CompletingEdit;

namespace Tw {
namespace Document {

class TextDocument : public QObject, public Document
{
	Q_OBJECT
	friend class ::CompletingEdit;
public:
	struct Tag {
		QTextCursor cursor;
		unsigned int level;
		QString text;
	};

	explicit TextDocument(QObject * parent = nullptr);
	explicit TextDocument(const QString & text, QObject * parent = nullptr);
	// Explicit out-of-line d'tor required by unique_ptr holding an incomplete type
	virtual ~TextDocument();

	const QList<Tag> & getTags() const { return _tags; }
	void addTag(const QTextCursor & cursor, const unsigned int level, const QString & text);
	unsigned int removeTags(int offset, int len);

	qsizetype length() const;
	bool isEmpty() const { return length() == 0; }

	QString line(const int line) const;
	int lineCount() const;

	virtual bool isModified() const;
	virtual void setModified(const bool modified = true);


signals:
	void tagsChanged() const;

protected:
	QList<Tag> _tags;
	std::unique_ptr<ScintillaDocument> m_scintilla;
};

} // namespace Document
} // namespace Tw

#endif // !defined(Document_TextDocument_H)
