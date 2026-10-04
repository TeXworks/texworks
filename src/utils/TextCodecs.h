/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2008-2025  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

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

#ifndef TWTextCodecs_H
#define TWTextCodecs_H

#include <QByteArray>
#include <QList>
#include <QString>

struct UConverter;

namespace Tw {
namespace Utils {

class TextCodec
{
public:
	TextCodec() = default;
	TextCodec(const QByteArray & name);
	virtual ~TextCodec();
	TextCodec(const TextCodec & other);
	TextCodec(TextCodec && other);
	TextCodec & operator=(const TextCodec & other);
	TextCodec & operator=(TextCodec && other);
	bool operator==(const TextCodec & other) const;
	bool operator!=(const TextCodec & other) const { return !(*this == other); }

	virtual QList<QByteArray> aliases() const;
	bool canEncode(const QString & str) const;
	virtual QByteArray name() const;
	virtual QString displayName() const;
	QString toUnicode(const QByteArray & a) const;
	QByteArray fromUnicode(const QString & str) const;
	virtual bool isValid() const;

	static QList<QByteArray> availableCodecs();
	static TextCodec codecForLocale();
protected:

private:
	UConverter * m_conv{nullptr};
};

} // namespace Utils
} // namespace Tw

#endif // !defined(TWTextCodecs)
