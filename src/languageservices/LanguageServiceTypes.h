/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICETYPES_H
#define LANGUAGESERVICETYPES_H

#include <QList>
#include <QMetaType>
#include <QString>
#include <QUrl>

namespace Tw {
namespace LanguageServices {

enum class TextSyncKind {
	None,
	Full,
	Incremental
};

struct LanguageServiceCapabilities
{
	TextSyncKind textSync{TextSyncKind::None};
	bool openClose{false};
	bool completion{false};
	bool signatureHelp{false};
	bool hover{false};
	bool definition{false};
	bool references{false};
	bool documentSymbols{false};
	bool workspaceSymbols{false};
	bool diagnostics{false};
};

struct LanguagePosition
{
	int line{0};
	int character{0};
};

struct LanguageRange
{
	LanguagePosition start;
	LanguagePosition end;
};

struct LanguageLocation
{
	QUrl document;
	LanguageRange range;
};

struct CompletionItem
{
	QString label;
	QString detail;
	QString documentation;
	QString insertText;
	bool hasReplacementRange{false};
	LanguageRange replacementRange;
};

struct LanguageCompletionRequest
{
	quint64 token{0};
	QUrl document;
	quint64 synchronizedVersion{0};
	LanguagePosition position;
};

struct LanguageDefinitionRequest
{
	quint64 token{0};
	QUrl document;
	quint64 synchronizedVersion{0};
	LanguagePosition position;
};

struct LanguageDocumentOpen
{
	QUrl url;
	QString languageId;
	quint64 version{1};
	QString text;
};

struct LanguageDocumentChange
{
	bool hasRange{false};
	LanguageRange range;
	QString text;
};

inline bool operator==(const LanguageServiceCapabilities & lhs, const LanguageServiceCapabilities & rhs)
{
	return lhs.textSync == rhs.textSync
	       && lhs.openClose == rhs.openClose
	       && lhs.completion == rhs.completion
	       && lhs.signatureHelp == rhs.signatureHelp
	       && lhs.hover == rhs.hover
	       && lhs.definition == rhs.definition
	       && lhs.references == rhs.references
	       && lhs.documentSymbols == rhs.documentSymbols
	       && lhs.workspaceSymbols == rhs.workspaceSymbols
	       && lhs.diagnostics == rhs.diagnostics;
}

inline bool operator!=(const LanguageServiceCapabilities & lhs, const LanguageServiceCapabilities & rhs)
{
	return !(lhs == rhs);
}

} // namespace LanguageServices
} // namespace Tw

Q_DECLARE_METATYPE(Tw::LanguageServices::TextSyncKind)
Q_DECLARE_METATYPE(Tw::LanguageServices::LanguageServiceCapabilities)
Q_DECLARE_METATYPE(Tw::LanguageServices::LanguageLocation)
Q_DECLARE_METATYPE(QList<Tw::LanguageServices::LanguageLocation>)
Q_DECLARE_METATYPE(Tw::LanguageServices::CompletionItem)
Q_DECLARE_METATYPE(QList<Tw::LanguageServices::CompletionItem>)

#endif // LANGUAGESERVICETYPES_H
