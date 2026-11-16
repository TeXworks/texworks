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

#ifndef DEFAULTFILEFILTERS_TEST_H
#define DEFAULTFILEFILTERS_TEST_H

#include <QtTest/QtTest>

namespace UnitTest {

class TestDefaultFileFilters : public QObject
{
	Q_OBJECT

private slots:
	void texDocumentClassification_data();
	void texDocumentClassification();
	void texDocumentFilter();
	void neighboringFiltersPreserved();
	void defaultFilterSelection();
	void defaultFilterSelectionIsCaseSensitive();
};

} // namespace UnitTest

#endif // DEFAULTFILEFILTERS_TEST_H
