/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  TeXworks contributors

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.
*/
#ifndef LANGUAGESERVICECONFIGURATION_H
#define LANGUAGESERVICECONFIGURATION_H

#include <QProcessEnvironment>
#include <QString>
#include <QStringList>

namespace Tw {
namespace LanguageServices {

struct LanguageServiceConfiguration
{
	QString executable;
	QStringList arguments;
	QProcessEnvironment environment{QProcessEnvironment::systemEnvironment()};
	QString workingDirectory;
};

} // namespace LanguageServices
} // namespace Tw

#endif // LANGUAGESERVICECONFIGURATION_H
