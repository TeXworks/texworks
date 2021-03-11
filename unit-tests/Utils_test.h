/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2021  Stefan Löffler

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

class TestUtils : public QObject
{
	Q_OBJECT
private slots:
	void initTestCase();
	void cleanupTestCase();

	void FileVersionDatabase_comparisons();
	void FileVersionDatabase_hashForFile();
	void FileVersionDatabase_addFileRecord();
	void FileVersionDatabase_load();
	void FileVersionDatabase_save();

	void SystemCommand_wait();
	void SystemCommand_getResult_data();
	void SystemCommand_getResult();

	void CommandLineParser_parse();
	void CommandLineParser_printUsage();

	void MacCentralEurRomanCodec();

	void FullscreenManager();

	void ResourcesLibrary_getLibraryPath_data();
	void ResourcesLibrary_getLibraryPath();
	void ResourcesLibrary_portableLibPath();

#ifdef Q_OS_DARWIN
	void OSVersionString();
#endif // defined(Q_OS_DARWIN)
};

} // namespace UnitTest
