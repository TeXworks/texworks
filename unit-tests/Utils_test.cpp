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

#include "Utils_test.h"

#include "SignalCounter.h"
#include "utils/CommandlineParser.h"
#include "utils/FileVersionDatabase.h"
#include "utils/SystemCommand.h"
#include "utils/TextCodecs.h"

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

void TestUtils::FileVersionDatabase_comparisons()
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

void TestUtils::FileVersionDatabase_hashForFile()
{
	QByteArray zero = QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e");

	QCOMPARE(Tw::Utils::FileVersionDatabase::hashForFile(QString("does-not-exist")), zero);
	QCOMPARE(Tw::Utils::FileVersionDatabase::hashForFile(QStringLiteral("base14-fonts.pdf")), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d"));
}

void TestUtils::FileVersionDatabase_addFileRecord()
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

void TestUtils::FileVersionDatabase_load()
{
	Tw::Utils::FileVersionDatabase db;
	db.addFileRecord(QFileInfo(QStringLiteral("/spaces test.tex")), QByteArray::fromHex("d41d8cd98f00b204e9800998ecf8427e"), QStringLiteral("v1"));
	db.addFileRecord(QFileInfo(QStringLiteral("base14-fonts.pdf")), QByteArray::fromHex("814514754a5680a57d172b6720d48a8d"), QStringLiteral("4.2"));

	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("does-not-exist")), Tw::Utils::FileVersionDatabase());
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("fileversion.db")), db);
	QEXPECT_FAIL("", "Invalid file version databases are not recognized", Continue);
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(QStringLiteral("script1.js")), Tw::Utils::FileVersionDatabase());
}

void TestUtils::FileVersionDatabase_save()
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

void TestUtils::SystemCommand_wait()
{
	Tw::Utils::SystemCommand cmd(this);
	SignalCounter spy(&cmd, SIGNAL(finished(int, QProcess::ExitStatus)));

	QVERIFY(spy.isValid());

	cmd.start(QStringLiteral("echo \"OK\""));
	QVERIFY(cmd.waitForStarted());
	QVERIFY(cmd.waitForFinished());

	spy.clear();

	cmd.start(QStringLiteral("echo \"OK\""));
	QVERIFY(spy.wait());
	QVERIFY(cmd.waitForStarted());
	QVERIFY(cmd.waitForFinished());
}

void TestUtils::SystemCommand_getResult_data()
{
	QTest::addColumn<QString>("program");
	QTest::addColumn<bool>("outputWanted");
	QTest::addColumn<bool>("runInBackground");
	QTest::addColumn<bool>("success");
	QTest::addColumn<QString>("output");

	QString progOK{QStringLiteral("echo \"OK\"")};
	QString progInvalid{QStringLiteral("invalid-command")};
	QString outputQuiet;
	QString outputOK{QStringLiteral("OK\n")};
	QString outputInvalid{QStringLiteral("ERROR: failure code 0")};

	QTest::newRow("success-quiet") << progOK << false << false << true << outputQuiet;
	QTest::newRow("success-quiet-background") << progOK << false << true << true << outputQuiet;
	QTest::newRow("success") << progOK << true << false << true << outputOK;
	QTest::newRow("success-background") << progOK << true << true << true << outputOK;

	QTest::newRow("invalid-quiet") << progInvalid << false << false << false << outputQuiet;
	QTest::newRow("invalid-quiet-background") << progInvalid << false << true << false << outputQuiet;
	QTest::newRow("invalid") << progInvalid << true << false << false << outputInvalid;
	QTest::newRow("invalid-background") << progInvalid << true << true << false << outputInvalid;
}

void TestUtils::SystemCommand_getResult()
{
	QFETCH(QString, program);
	QFETCH(bool, outputWanted);
	QFETCH(bool, runInBackground);
	QFETCH(bool, success);
	QFETCH(QString, output);

	Tw::Utils::SystemCommand * cmd = new Tw::Utils::SystemCommand(this, outputWanted, runInBackground);
	QSignalSpy spy(cmd, SIGNAL(destroyed()));

	QVERIFY(spy.isValid());

	cmd->start(program);
	QCOMPARE(cmd->waitForFinished(), success);
	QCOMPARE(cmd->getResult(), output);

	if (!runInBackground) {
		cmd->deleteLater();
	}
	QVERIFY(spy.wait());
}

void TestUtils::CommandLineParser_parse()
{
	QString exe{QStringLiteral("exe")};
	{
		Tw::Utils::CommandlineParser clp(QStringList({exe}));
		QVERIFY(clp.parse());
		QCOMPARE(clp.getNextArgument(), -1);
		QCOMPARE(clp.getPrevArgument(), -1);
		QCOMPARE(clp.getNextOption(), -1);
		QCOMPARE(clp.getPrevOption(), -1);
		QCOMPARE(clp.getNextSwitch(), -1);
		QCOMPARE(clp.getPrevSwitch(), -1);
	}
	{
		Tw::Utils::CommandlineParser clp(QStringList({exe, QStringLiteral("--oLong=asdf"), QStringLiteral("-o=ghjk"), QStringLiteral("qwerty"), QStringLiteral("-s"), QStringLiteral("--sLong")}));
		clp.registerSwitch(QStringLiteral("sLong"), QString(), QStringLiteral("s"));
		clp.registerOption(QStringLiteral("oLong"), QString(), QStringLiteral("o"));
		QVERIFY(clp.parse());
		int nextArg = clp.getNextArgument();
		QCOMPARE(nextArg, 2);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextArg);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Argument);
			QCOMPARE(item.longName, QString());
			QCOMPARE(item.value, QVariant(QStringLiteral("qwerty")));
			QVERIFY(item.processed == false);
		}
		int nextOpt = clp.getNextOption(QStringLiteral("oLong"));
		QCOMPARE(nextOpt, 0);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextOpt);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Option);
			QCOMPARE(item.longName, QStringLiteral("oLong"));
			QCOMPARE(item.value, QVariant(QStringLiteral("asdf")));
			QVERIFY(item.processed == false);
		}
		int nextOpt2 = clp.getNextOption(QStringLiteral("oLong"), nextOpt);
		QCOMPARE(nextOpt2, 1);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextOpt2);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Option);
			QCOMPARE(item.longName, QStringLiteral("oLong"));
			QCOMPARE(item.value, QVariant(QStringLiteral("ghjk")));
			QVERIFY(item.processed == false);
		}
		QCOMPARE(clp.getNextOption(QStringLiteral("oLong"), nextOpt2), -1);

		int nextSwitch = clp.getNextSwitch(QStringLiteral("sLong"));
		QCOMPARE(nextSwitch, 3);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextSwitch);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Switch);
			QCOMPARE(item.longName, QStringLiteral("sLong"));
			QCOMPARE(item.value, QVariant());
			QVERIFY(item.processed == false);
		}
		int nextSwitch2 = clp.getNextSwitch(QStringLiteral("sLong"), nextSwitch);
		QCOMPARE(nextSwitch2, 4);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextSwitch2);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Switch);
			QCOMPARE(item.longName, QStringLiteral("sLong"));
			QCOMPARE(item.value, QVariant());
			QVERIFY(item.processed == false);
		}
		QCOMPARE(clp.getNextSwitch(QStringLiteral("sLong"), nextSwitch2), -1);

		QCOMPARE(clp.getNextOption(QStringLiteral("sLong")), -1);
		QCOMPARE(clp.getNextSwitch(QStringLiteral("oLong")), -1);

		QCOMPARE(clp.getPrevArgument(), -1);
		QCOMPARE(clp.getPrevArgument(nextArg), -1);
		QCOMPARE(clp.getPrevArgument(nextArg + 1), nextArg);
		QCOMPARE(clp.getPrevOption(QStringLiteral("oLong")), -1);
		QCOMPARE(clp.getPrevOption(QStringLiteral("oLong"), nextOpt), -1);
		QCOMPARE(clp.getPrevOption(QStringLiteral("oLong"), nextOpt + 1), nextOpt);
		QCOMPARE(clp.getPrevOption(QStringLiteral("oLong"), nextOpt2 + 1), nextOpt2);
		QCOMPARE(clp.getPrevSwitch(QStringLiteral("sLong")), -1);
		QCOMPARE(clp.getPrevSwitch(QStringLiteral("sLong"), nextSwitch), -1);
		QCOMPARE(clp.getPrevSwitch(QStringLiteral("sLong"), nextSwitch + 1), nextSwitch);
		QCOMPARE(clp.getPrevSwitch(QStringLiteral("sLong"), nextSwitch2 + 1), nextSwitch2);
	}
}

void TestUtils::CommandLineParser_printUsage()
{
	Tw::Utils::CommandlineParser clp;
	QString buffer;
	QTextStream strm(&buffer);
	clp.registerSwitch(QStringLiteral("sLong"), QStringLiteral("sDesc"), QStringLiteral("s"));
	clp.registerOption(QStringLiteral("oLong"), QStringLiteral("oDesc"), QStringLiteral("o"));

	clp.printUsage(strm);

	QCOMPARE(strm.readAll(), QStringLiteral("Usage: %1 [opts/args]\n\n   --sLong, -s   sDesc\n   --oLong=..., -o=...   oDesc\n").arg(QFileInfo(QCoreApplication::applicationFilePath()).fileName()));
}

void TestUtils::MacCentralEurRomanCodec()
{
	Tw::Utils::MacCentralEurRomanCodec * c = Tw::Utils::MacCentralEurRomanCodec::instance();
	Q_ASSERT(c != nullptr);

	QCOMPARE(c->mibEnum(), -4000);
	QCOMPARE(c->name(), QByteArray("Mac Central European Roman"));
	QCOMPARE(c->aliases(), QList<QByteArray>({"MacCentralEuropeanRoman", "MacCentralEurRoman"}));

	// € cannot be encoded and is replaced by ? (or \x00 if
	// QTextCodec::ConvertInvalidToNull is specified)
	QCOMPARE(c->fromUnicode(QStringLiteral("AÄĀ°§€")), QByteArray("\x41\x80\x81\xA1\xA4?"));
	QCOMPARE(c->toUnicode(QByteArray("\x41\x80\x81\xA1\xA4")), QStringLiteral("AÄĀ°§"));

	QCOMPARE(c->toUnicode(nullptr), QString());

	QTextEncoder * e = c->makeEncoder(QTextCodec::ConvertInvalidToNull);
	QCOMPARE(e->fromUnicode(QStringLiteral("AÄĀ°§€")), QByteArray("\x41\x80\x81\xA1\xA4\x00", 6));
	delete e;
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestUtils)
