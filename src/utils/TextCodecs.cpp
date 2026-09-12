/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2012-2025  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

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

#include "utils/TextCodecs.h"

#include <unicode/ucnv.h>
#include <unicode/errorcode.h>
#include <unicode/uniset.h>

namespace Tw {
namespace Utils {

// static
QList<QByteArray> TextCodec::aliases() const
{
	QList<QByteArray> retVal;
	QByteArray name = this->name();
	icu::ErrorCode err;
	for (uint16_t iAlias = 0; iAlias < ucnv_countAliases(name.constData(), err); ++iAlias) {
		retVal.append(ucnv_getAlias(name.constData(), iAlias, err));
	}
	return retVal;
}

bool TextCodec::canEncode(const QString & str) const
{
	// TODO: for short strings (and multi-byte encodings) it might be faster
	// (and more memory efficient) to just try to convert the string and see if
	// that works
	icu::ErrorCode err;
	icu::UnicodeSet set;

	ucnv_getUnicodeSet(m_conv, set.toUSet(), UCNV_ROUNDTRIP_SET, err);

	return set.containsAll(icu::UnicodeString(str.utf16(), static_cast<int32_t>(str.size())));
}

QByteArray TextCodec::name() const
{
	icu::ErrorCode err;
	return ucnv_getName(m_conv, err);
}

QString TextCodec::displayName() const
{
	return QString::fromLatin1(name());
}

QString TextCodec::toUnicode(const QByteArray &a) const
{
	const int BufferSize = 65536;

	UChar buffer[BufferSize];
	const char * source = a.constData();
	const char * sourceLimit = source + a.size();
	QString retVal;
	icu::ErrorCode err;

	while (source != sourceLimit && (err.isSuccess() || err.get() == U_BUFFER_OVERFLOW_ERROR)) {
		err.reset();
		UChar * target = buffer;
		UChar * targetLimit = &buffer[BufferSize];
		ucnv_toUnicode(m_conv, &target, targetLimit, &source, sourceLimit, nullptr, true, err);
		retVal += QString::fromUtf16(buffer, target - buffer);
	}
	return retVal;
}

QByteArray TextCodec::fromUnicode(const QString &str) const
{
	const int BufferSize = 65536;
	char buffer[BufferSize];

	const UChar * source = reinterpret_cast<const UChar*>(str.utf16());
	const UChar * sourceLimit = source + str.size();
	QByteArray retVal;
	icu::ErrorCode err;

	while (source != sourceLimit && (err.isSuccess() || err.get() == U_BUFFER_OVERFLOW_ERROR)) {
		err.reset();
		char * target = buffer;
		char * targetLimit = &buffer[BufferSize];
		ucnv_fromUnicode(m_conv, &target, targetLimit, &source, sourceLimit, nullptr, true, err);
		retVal.append(buffer, target - buffer);
	}
	return retVal;
}

QList<QByteArray> TextCodec::availableCodecs()
{
	QList<QByteArray> retVal;
	for (int32_t iName = 0; iName < ucnv_countAvailable(); ++iName) {
		auto name = ucnv_getAvailableName(iName);
		icu::ErrorCode err;
		for (uint16_t iAlias = 0; iAlias < ucnv_countAliases(name, err); ++iAlias) {
			retVal.append(ucnv_getAlias(name, iAlias, err));
		}
	}
	return retVal;
}

// static
TextCodec *TextCodec::codecForName(const QByteArray &name)
{
	icu::ErrorCode err;
	UConverter * conv{ucnv_open(name.constData(), err)};

	if (conv == nullptr) {
		return nullptr;
	}

	TextCodec * retVal = new TextCodec();
	retVal->m_conv = conv;
	return retVal;
}

// static
TextCodec *TextCodec::codecForLocale()
{
	return codecForName(ucnv_getDefaultName());
}

TextCodec::~TextCodec()
{
	ucnv_close(m_conv);
}

} // namespace Utils
} // namespace Tw
