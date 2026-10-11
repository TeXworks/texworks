/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICENAVIGATION_H
#define LANGUAGESERVICENAVIGATION_H

#include "languageservices/LanguageServiceTypes.h"

#include <QTextCursor>

class QTextDocument;

namespace Tw {
namespace LanguageServices {

bool cursorForLanguageRange(QTextDocument * document, const LanguageRange & range, QTextCursor & cursor);

} // namespace LanguageServices
} // namespace Tw

#endif // LANGUAGESERVICENAVIGATION_H
