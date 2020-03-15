/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019  Stefan Löffler

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
#include "FileVersionDatabase_test.h"
#include "utils/FileVersionDatabase.h"

#include <QTemporaryFile>

namespace Tw {
namespace Utils {
bool operator==(const FileVersionDatabase::Record & r1, const FileVersionDatabase::Record & r2)
{
	if (r1.version != r2.version || r1.hash != r2.hash)
		return false;

	// Work around the fact that the behavior of QFileInfo::operator== is
	// undefined if the objects are empty or refer to non-existant files

	// If the only one of the files exists, they are clearly not the same
	if (r1.filePath.exists() != r2.filePath.exists())
		return false;

	if (r1.filePath.exists()) {
		// If they exist, we can compare them directly
		return (r1.filePath == r2.filePath);
	}
	else {
		// If they don't exist, we compare the stored absolute file paths
		// (which can be empty)
		return (r1.filePath.absolutePath() == r2.filePath.absolutePath());
	}
}
bool operator==(const FileVersionDatabase & db1, const FileVersionDatabase & db2) {
	return db1.getFileRecords() == db2.getFileRecords();
}
} // namespace Utils
} // namespace Tw


namespace UnitTest {

void TestFileVersionDatabase::comparisons()
{
	Tw::Utils::FileVersionDatabase::Record r1 = {QFileInfo(QStringLiteral("base14-fonts.pdf")), QStringLiteral("v1"), QByteArray()};
	Tw::Utils::FileVersionDatabase::Record r2 = {QFileInfo(QStringLiteral("base14-fonts.pdf")), QString(), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d")};
	Tw::Utils::FileVersionDatabase::Record r3 = {QFileInfo(QStringLiteral("does-not=exist")), QString(), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d")};

	QVERIFY(r1 == r1);
	QVERIFY(r2 == r2);
	QVERIFY(r3 == r3);
	QVERIFY(!(r1 == r2));
	QVERIFY(!(r1 == r3));
	QVERIFY(!(r2 == r3));
}

void TestFileVersionDatabase::hashForFile()
{
	QByteArray zero = QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e");

	QCOMPARE(Tw::Utils::FileVersionDatabase::hashForFile(QString("does-not-exist")), zero);
	QCOMPARE(Tw::Utils::FileVersionDatabase::hashForFile(QStringLiteral("base14-fonts.pdf")), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d"));
}

void TestFileVersionDatabase::addFileRecord()
{
	Tw::Utils::FileVersionDatabase db;
	Tw::Utils::FileVersionDatabase::Record empty = {QFileInfo(), QString(), QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e")};
	Tw::Utils::FileVersionDatabase::Record r1 = {QFileInfo(QStringLiteral("base14-fonts.pdf")), QStringLiteral("v1"), QByteArray()};
	Tw::Utils::FileVersionDatabase::Record r2 = {QFileInfo(QStringLiteral("base14-fonts.pdf")), QString(), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d")};

	QVERIFY(db.hasFileRecord(r1.filePath) == false);
	QCOMPARE(db.getFileRecord(r1.filePath), empty);
	QCOMPARE(db.getFileRecords(), QList<Tw::Utils::FileVersionDatabase::Record>());
	db.addFileRecord(r1.filePath, r1.hash, r1.version);
	QVERIFY(db.hasFileRecord(r1.filePath));
	QCOMPARE(db.getFileRecord(r1.filePath), r1);
	QCOMPARE(db.getFileRecords(), QList<Tw::Utils::FileVersionDatabase::Record>{r1});
	db.addFileRecord(r2.filePath, r2.hash, r2.version);
	QVERIFY(db.hasFileRecord(r2.filePath));
	QCOMPARE(db.getFileRecord(r2.filePath), r2);
	QCOMPARE(db.getFileRecords(), QList<Tw::Utils::FileVersionDatabase::Record>{r2});
}

void TestFileVersionDatabase::load()
{
	Tw::Utils::FileVersionDatabase db;
	db.addFileRecord(QFileInfo(QStringLiteral("/spaces test.tex")), QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e"), QStringLiteral("v1"));
	db.addFileRecord(QFileInfo(QStringLiteral("base14-fonts.pdf")), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d"), QStringLiteral("4.2"));

	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("does-not-exist")), Tw::Utils::FileVersionDatabase());
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("fileversion.db")), db);
	QEXPECT_FAIL("", "Invalid file version databases are not recognized", Continue);
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("script1.js")), Tw::Utils::FileVersionDatabase());
}

void TestFileVersionDatabase::save()
{
	Tw::Utils::FileVersionDatabase db;
	db.addFileRecord(QFileInfo(QStringLiteral("/spaces test.tex")), QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e"), QStringLiteral("v1"));
	db.addFileRecord(QFileInfo(QStringLiteral("base14-fonts.pdf")), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d"), QStringLiteral("4.2"));

	QVERIFY(db.save(QStringLiteral("does/not/exist.db")) == false);

	QTemporaryFile tmpFile;
	tmpFile.open();
	tmpFile.close();
	QVERIFY(db.save(tmpFile.fileName()));
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(tmpFile.fileName()), db);
}



} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

QTEST_MAIN(UnitTest::TestFileVersionDatabase)
