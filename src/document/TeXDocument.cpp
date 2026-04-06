/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2022  Stefan Löffler

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

#include "document/TeXDocument.h"
#include "TeXHighlighter.h"

#include "../utils/CompilerWarnings.h"
WARNINGS_PUSH()
WARNINGS_DISABLE("-Wzero-as-null-pointer-constant")
#include <ScintillaDocument.h>
#include <ScintillaTypes.h>
WARNINGS_POP()

#include <QDir>
#include <QFileInfo>
#include <QTextCodec>

namespace {
const std::unordered_map<std::string, const char *> texshopSynonyms = {
	{"macosroman",			"Apple Roman"},
	{"isolatin",			"ISO 8859-1"},
	{"isolatin2",			"ISO 8859-2"},
	{"isolatin5",			"ISO 8859-5"},
	{"isolatin9",			"ISO 8859-9"},
	//	{"macjapanese",		""},
	//	{"dosjapanese",		""},
	{"sjis_x0213",			"Shift-JIS"},
	{"euc_jp",				"EUC-JP"},
	//	{"jisjapanese",		""},
	//	{"mackorean",		""},
	{"utf-8 unicode",		"UTF-8"},
	{"standard unicode",	"UTF-16"},
	//	{"mac cyrillic",		""},
	//	{"dos cyrillic",		""},
	//	{"dos russian",		""},
	{"windows cyrillic",	"Windows-1251"},
	{"koi8_r",				"KOI8-R"},
	//	{"mac chinese traditional",	""},
	//	{"mac chinese simplified",	""},
	//	{"dos chinese traditional",	""},
	//	{"dos chinese simplified",	""},
	//	{"gbk",				""},
	//	{"gb 2312",			""},
	{"gb 18030",			"GB18030-0"}
};
}

namespace Tw {
namespace Document {

TeXDocument::TeXDocument(QObject * parent) : TextDocument(parent)
{
	/* FIXME
	connect(this, &TeXDocument::contentsChange, this, &TeXDocument::maybeUpdateModeLines);
*/
}

TeXDocument::TeXDocument(const QString & text, QObject * parent) : TextDocument(text, parent)
{
	/* FIXME
	connect(this, &TeXDocument::contentsChange, this, &TeXDocument::maybeUpdateModeLines);
*/
	parseModeLines();
}

TeXHighlighter * TeXDocument::getHighlighter() const
{
	return findChild<TeXHighlighter*>();
}

void TeXDocument::parseModeLines()
{
	QMap<QString, QString> newModeLines;

	QRegularExpression re(QStringLiteral(u"%(?:\\^\\^A)?\\s*!TEX\\s+(?:TS-)?(\\w+)\\s*=\\s*([^\r\n\x2029]+)[\r\n\x2029]"), QRegularExpression::CaseInsensitiveOption);

	const QString peekText = QString::fromUtf8(m_scintilla->get_char_range(0, qMin(PeekLength, m_scintilla->length())));
	QRegularExpressionMatchIterator it = re.globalMatch(peekText);

	while (it.hasNext()) {
		QRegularExpressionMatch m = it.next();
		newModeLines.insert(m.captured(1).trimmed().toLower(), m.captured(2).trimmed());
	}

	if (_modelines != newModeLines) {
		QStringList changedKeys;
		QStringList removedKeys;

		Q_FOREACH(QString key, _modelines.keys()) {
			if (!newModeLines.contains(key)) {
				removedKeys.append(key);
			}
		}
		Q_FOREACH(QString key, newModeLines.keys()) {
			if (!_modelines.contains(key) || _modelines.value(key) != newModeLines.value(key)) {
				changedKeys.append(key);
			}
		}

		_modelines = newModeLines;
		emit modelinesChanged(changedKeys, removedKeys);
	}
}

QString TeXDocument::getRootFilePath() const
{
	if (hasModeLine(QStringLiteral("root"))) {
		const QString rootName{getModeLineValue(QStringLiteral("root")).trimmed()};
		const QFileInfo rootFileInfo{getFileInfo().dir(), rootName};
		return rootFileInfo.absoluteFilePath();
	}

	if (!isStoredInFilesystem()) {
		return {};
	}
	return absoluteFilePath();
}

void TeXDocument::maybeUpdateModeLines(int position, int charsRemoved, int charsAdded)
{
	Q_UNUSED(charsRemoved)
	Q_UNUSED(charsAdded)

	if (position < PeekLength)
		parseModeLines();
}

void TeXDocument::guessReadSettings(const QFileInfo &path, FileSettings &settings, const QByteArray &peekBytes)
{
	if (settings.codec != nullptr) {
		TextDocument::guessReadSettings(path, settings, peekBytes);
		return;
	}

	const QString peekStr = QString::fromLatin1(peekBytes);

	// peek at the file for %!TEX encoding = ....
	QRegularExpression re(QStringLiteral(u"% *!TEX +encoding *= *([^\r\n\x2029]+)[\r\n\x2029]"), QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch m = re.match(peekStr);
	if (m.hasMatch()) {
		const QString reqName = m.captured(1).trimmed();
		settings.codec = QTextCodec::codecForName(reqName.toLatin1());
		if (settings.codec == nullptr) {
			const auto it = texshopSynonyms.find(reqName.toLower().toStdString());
			if (it != texshopSynonyms.cend()) {
				settings.codec = QTextCodec::codecForName(it->second);
			}
			else if(settings.ignoreUnsupportedEncoding == false) {
				throw UnsupportedEncodingException(reqName);
			}
		}
	}
	TextDocument::guessReadSettings(path, settings, peekBytes);
}

// static
bool TeXDocument::findNextWord(const QString & text, const QString::size_type index, QString::size_type & start, QString::size_type & end)
{
	// try to do a sensible "word" selection for TeX documents, taking into
	// account the form of control sequences:
	// given an index representing a caret,
	// - if current char (following caret) is a letter, apostrophe, or '@',
	//   extend in both directions
	//   - include apostrophe if surrounded by letters
	//   - include preceding backslash if any, unless word contains apostrophe
	// - if preceeding char is a \, extend to include \ only
	// - if current char is a number, extend in both directions
	// - if current char is a space or tab, extend in both directions to include
	//   all spaces or tabs
	// - if current char is a \, include next char; if letter or '@', extend to
	//   include all following letters or '@'
	// - else select single char following index
	// returns TRUE if the resulting selection consists of word-forming chars

	start = end = index;

	if (text.length() < 1) // empty
		return false;
	if (index >= text.length()) // end of line
		return false;

	QChar ch = text.at(index);

	auto isWordForming = [](const QChar & c) { return c.isLetter() || c.isMark(); };

	if (isWordForming(ch) || ch == QChar::fromLatin1('@') /* || ch == QChar::fromLatin1('\'') || ch == 0x2019 */) {
		bool isControlSeq{false}; // becomes true if we include an @ sign or a leading backslash
		bool includesApos{false}; // becomes true if we include an apostrophe
		if (ch == QChar::fromLatin1('@'))
			isControlSeq = true;
		//else if (ch == QChar::fromLatin1('\'') || ch == 0x2019)
		//	includesApos = true;
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (isWordForming(ch))
				continue;
			if (!includesApos && ch == QChar::fromLatin1('@')) {
				isControlSeq = true;
				continue;
			}
			if (!isControlSeq && (ch == QChar::fromLatin1('\'') || ch == QChar(0x2019)) && start > 0 && isWordForming(text.at(start - 1))) {
				includesApos = true;
				continue;
			}
			++start;
			break;
		}
		if (start > 0 && text.at(start - 1) == QChar::fromLatin1('\\')) {
			isControlSeq = true;
			--start;
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (isWordForming(ch))
				continue;
			if (!includesApos && ch == QChar::fromLatin1('@')) {
				isControlSeq = true;
				continue;
			}
			if (!isControlSeq && (ch == QChar::fromLatin1('\'') || ch == QChar(0x2019)) && end < text.length() - 1 && isWordForming(text.at(end + 1))) {
				includesApos = true;
				continue;
			}
			break;
		}
		return !isControlSeq;
	}

	if (index > 0 && text.at(index - 1) == QChar::fromLatin1('\\')) {
		start = index - 1;
		end = index + 1;
		return false;
	}

	if (ch.isNumber()) {
		// TODO: handle decimals, leading signs
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (ch.isNumber())
				continue;
			++start;
			break;
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (ch.isNumber())
				continue;
			break;
		}
		return false;
	}

	if (ch == QChar::fromLatin1(' ') || ch == QChar::fromLatin1('\t')) {
		while (start > 0) {
			--start;
			ch = text.at(start);
			if (!(ch == QChar::fromLatin1(' ') || ch == QChar::fromLatin1('\t'))) {
				++start;
				break;
			}
		}
		while (++end < text.length()) {
			ch = text.at(end);
			if (!(ch == QChar::fromLatin1(' ') || ch == QChar::fromLatin1('\t')))
				break;
		}
		return false;
	}

	if (ch == QChar::fromLatin1('\\')) {
		if (++end < text.length()) {
			ch = text.at(end);
			if (isWordForming(ch) || ch == QChar::fromLatin1('@'))
				while (++end < text.length()) {
					ch = text.at(end);
					if (isWordForming(ch) || ch == QChar::fromLatin1('@'))
						continue;
					break;
				}
			else
				++end;
		}
		return false;
	}

	// else the character is selected in isolation
	end = index + 1;
	return false;
}

} // namespace Document
} // namespace Tw
