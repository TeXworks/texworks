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

#include "DefaultFileFilters_test.h"
#include "DefaultFileFilters.h"

#include <QDir>
#include <QFileInfo>

namespace UnitTest {

void TestDefaultFileFilters::texDocumentClassification_data()
{
	QTest::addColumn<QString>("fileName");
	QTest::addColumn<bool>("isTeXDocument");

	QTest::newRow("tex") << QStringLiteral("example.tex") << true;
	QTest::newRow("mkiv") << QStringLiteral("example.mkiv") << true;
	QTest::newRow("mkxl") << QStringLiteral("example.mkxl") << true;
	QTest::newRow("uppercase-mkxl") << QStringLiteral("example.MKXL") << true;
	QTest::newRow("unrelated") << QStringLiteral("example.cpp") << false;
}

void TestDefaultFileFilters::texDocumentClassification()
{
	QFETCH(QString, fileName);
	QFETCH(bool, isTeXDocument);

	QCOMPARE(QDir::match(Tw::DefaultFileFilters::texDocumentNameFilters(), QFileInfo(fileName).fileName()), isTeXDocument);
}

void TestDefaultFileFilters::texDocumentFilter()
{
	const QStringList filters{Tw::DefaultFileFilters::filters()};
	QVERIFY(!filters.isEmpty());
	const QString & texFilter{filters.first()};

	const QString::size_type texPosition{texFilter.indexOf(QStringLiteral("*.tex"))};
	const QString::size_type mkivPosition{texFilter.indexOf(QStringLiteral("*.mkiv"))};
	const QString::size_type mkxlPosition{texFilter.indexOf(QStringLiteral("*.mkxl"))};
	QVERIFY(texPosition >= 0);
	QVERIFY(mkivPosition > texPosition);
	QVERIFY(mkxlPosition > mkivPosition);
}

void TestDefaultFileFilters::neighboringFiltersPreserved()
{
	const QStringList filters{Tw::DefaultFileFilters::filters()};
	QVERIFY(filters.contains(QObject::tr("LaTeX documents (*.ltx)")));
	QVERIFY(filters.contains(QObject::tr("BibTeX databases (*.bib)")));
	QVERIFY(filters.contains(QObject::tr("Style files (*.sty)")));
	QVERIFY(filters.contains(QObject::tr("Class files (*.cls)")));
	QVERIFY(filters.contains(QObject::tr("Documented macros (*.dtx)")));
	QVERIFY(filters.contains(QObject::tr("PDF documents (*.pdf)")));
}

void TestDefaultFileFilters::defaultFilterSelection()
{
	const QStringList filters{Tw::DefaultFileFilters::filters()};
	QCOMPARE(Tw::DefaultFileFilters::chooseForFile(QStringLiteral("example.tex"), filters), filters.first());
	QCOMPARE(Tw::DefaultFileFilters::chooseForFile(QStringLiteral("example.mkiv"), filters), filters.last());
	QCOMPARE(Tw::DefaultFileFilters::chooseForFile(QStringLiteral("example.mkxl"), filters), filters.last());
	QCOMPARE(Tw::DefaultFileFilters::chooseForFile(QStringLiteral("example.ltx"), filters), QObject::tr("LaTeX documents (*.ltx)"));
}

void TestDefaultFileFilters::defaultFilterSelectionIsCaseSensitive()
{
	const QStringList filters{Tw::DefaultFileFilters::filters()};
	QCOMPARE(Tw::DefaultFileFilters::chooseForFile(QStringLiteral("example.TEX"), filters), filters.last());
}

} // namespace UnitTest

QTEST_APPLESS_MAIN(UnitTest::TestDefaultFileFilters)
