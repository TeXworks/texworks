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

#include <QMetaType>

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

#endif // LANGUAGESERVICETYPES_H
