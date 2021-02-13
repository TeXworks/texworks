/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2020  Stefan Löffler

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.

	For links to further information, or to contact the authors,
	see <http://www.tug.org/texworks/>.
*/

#include <QtTest/QtTest>

namespace UnitTest {

class TestDocument : public QObject
{
	Q_OBJECT
private slots:
	void isPDFfile_data();
	void isPDFfile();
	void isImageFile_data();
	void isImageFile();
	void isPostscriptFile_data();
	void isPostscriptFile();

	void fileInfo();
	void storedInFilesystem();
	void absoluteFilePath();

	void tags();

	void getHighlighter();
	void modelines();
	void findNextWord_data();
	void findNextWord();

	void SpellChecker_getDictionaryList();
	void SpellChecker_getDictionary();
	void SpellChecker_ignoreWord();

	void Synchronizer_isValid();
	void syncTeXFilename();
	void pdfFilename();

	void Synchronizer_syncFromTeX_data();
	void Synchronizer_syncFromTeX();
	void Synchronizer_syncFromPDF_data();
	void Synchronizer_syncFromPDF();
};

} // namespace UnitTest
