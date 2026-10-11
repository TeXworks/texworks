/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2026  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen

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

#include "DefaultFileFilters.h"

#include <QFileInfo>
#include <QObject>

namespace Tw {
namespace DefaultFileFilters {

QStringList texDocumentNameFilters()
{
	return QStringList()
	    << QStringLiteral("*.tex")
	    << QStringLiteral("*.mkiv")
	    << QStringLiteral("*.mkxl");
}

QStringList filters()
{
	QStringList defaultFilters;
	QString texDocumentFilter{QObject::tr("TeX documents (*.tex)")};
	texDocumentFilter.replace(QStringLiteral("*.tex"), texDocumentNameFilters().join(QLatin1Char(' ')));
	defaultFilters << texDocumentFilter;
	defaultFilters << QObject::tr("LaTeX documents (*.ltx)");
	defaultFilters << QObject::tr("Log files (*.log *.blg)");
	defaultFilters << QObject::tr("BibTeX databases (*.bib)");
	defaultFilters << QObject::tr("Style files (*.sty)");
	defaultFilters << QObject::tr("Class files (*.cls)");
	defaultFilters << QObject::tr("Documented macros (*.dtx)");
	defaultFilters << QObject::tr("Auxiliary files (*.aux *.toc *.lot *.lof *.nav *.out *.snm *.ind *.idx *.bbl *.brf)");
	defaultFilters << QObject::tr("Text files (*.txt)");
	defaultFilters << QObject::tr("PDF documents (*.pdf)");
#ifdef Q_OS_WIN
	// It seems (contrary to documentation) that on Windows, this has to be
	// *.* to allow saving files with non-standard extensions (though *.* still
	// does not allow to save without any extension at all, see below)
	const QString allFilesFilter = QStringLiteral("*.*");
//	const QString allFilesFilter = QStringLiteral("*");
#else
	// On other systems, *.* might require a . in the filename, which would
	// preclude filenames without extension. In line with the documentation, *
	// should be used in those cases
	const QString allFilesFilter = QStringLiteral("*");
#endif
	defaultFilters << QObject::tr("All files") + QStringLiteral(" (%1)").arg(allFilesFilter);
	return defaultFilters;
}

QString chooseForFile(const QString & filename, const QStringList & filters)
{
	QString extension = QFileInfo(filename).completeSuffix();

	if (extension.isEmpty())
		return filters.last();

	foreach (QString filter, filters) {
		// return filter if it corresponds to the given extension
		// note that the extension must be the first one in the list to match;
		// otherwise, the file dialog would replace the actual extension by the
		// first one in the list, thereby altering it without cause
		if (filter.contains(QString::fromLatin1("(*.%1").arg(extension)))
			return filter;
	}
	// if no filter matched, return the last one (which should be "All files")
	return filters.last();
}

} // namespace DefaultFileFilters
} // namespace Tw
