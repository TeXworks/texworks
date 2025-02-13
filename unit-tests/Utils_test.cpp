/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2019-2024  Stefan Löffler

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

#include "utils/CommandlineParser.h"
#include "utils/FileVersionDatabase.h"
#include "utils/FullscreenManager.h"
#include "utils/ResourcesLibrary.h"
#include "utils/SystemCommand.h"
#include "utils/TextCodecs.h"
#include "utils/TypesetManager.h"

#include <QMenuBar>
#include <QMouseEvent>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QToolBar>

#ifdef Q_OS_DARWIN
extern QString GetMacOSVersionString();
#endif // defined(Q_OS_DARWIN)

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

class FullscreenManager : public Tw::Utils::FullscreenManager
{
public:
	explicit FullscreenManager(QMainWindow * parent) : Tw::Utils::FullscreenManager(parent) {
		_menuBarTimer.setInterval(100);
	}
	int timeout() const { return _menuBarTimer.interval(); }
	QList<shortcut_info> shortcuts() const { return _shortcuts; }
};

void TestUtils::initTestCase()
{
	QStandardPaths::setTestModeEnabled(true);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
	qRegisterMetaType<QProcess::ExitStatus>("QProcess::ExitStatus");
#endif
}

void TestUtils::cleanupTestCase()
{
	QStandardPaths::setTestModeEnabled(false);
}

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

	QTemporaryDir tmpDir;
#if QT_VERSION < QT_VERSION_CHECK(5, 9, 0)
	const QString tmpFile{QDir(tmpDir.path()).filePath(QStringLiteral("db"))};
#else
	const QString tmpFile{tmpDir.filePath(QStringLiteral("db"))};
#endif
	QVERIFY(db.save(tmpFile));
	QCOMPARE(Tw::Utils::FileVersionDatabase::load(tmpFile), db);
}

void TestUtils::SystemCommand_wait()
{
	Tw::Utils::SystemCommand cmd(this);
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&cmd, SIGNAL(finished(int, QProcess::ExitStatus)));
#else
	QSignalSpy spy(&cmd, static_cast<void (Tw::Utils::SystemCommand::*)(int, QProcess::ExitStatus)>(&Tw::Utils::SystemCommand::finished));
#endif

	QVERIFY(spy.isValid());

#ifdef Q_OS_WINDOWS
	cmd.start(QStringLiteral("cmd"), QStringList{QStringLiteral("/C"), QStringLiteral("echo OK")});
#else
	cmd.start(QStringLiteral("echo"), QStringList{QStringLiteral("OK")});
#endif
	QVERIFY(cmd.waitForStarted());
	QVERIFY(cmd.waitForFinished());

	spy.clear();

#ifdef Q_OS_WINDOWS
	cmd.start(QStringLiteral("cmd"), QStringList{QStringLiteral("/C"), QStringLiteral("echo OK")});
#else
	cmd.start(QStringLiteral("echo"), QStringList{QStringLiteral("OK")});
#endif
	QVERIFY(spy.wait());
	QVERIFY(cmd.waitForStarted());
	QVERIFY(cmd.waitForFinished());
}

void TestUtils::SystemCommand_getResult_data()
{
	QTest::addColumn<QString>("program");
	QTest::addColumn<QStringList>("args");
	QTest::addColumn<bool>("outputWanted");
	QTest::addColumn<bool>("runInBackground");
	QTest::addColumn<bool>("success");
	QTest::addColumn<QString>("output");
#ifdef Q_OS_WINDOWS
	QString progOK{QStringLiteral("cmd")};
	QStringList progOKArgs{QStringLiteral("/C"), QStringLiteral("echo OK")};
#else
	QString progOK{QStringLiteral("echo")};
	QStringList progOKArgs{QStringLiteral("OK")};
#endif
	QString progInvalid{QStringLiteral("invalid-command")};
	QStringList progInvalidArgs{};
	QString outputQuiet;
	QString outputOK{QStringLiteral("OK\n")};
	QString outputInvalid{QStringLiteral("ERROR: failure code 0")};

	QTest::newRow("success-quiet") << progOK << progOKArgs << false << false << true << outputQuiet;
	QTest::newRow("success-quiet-background") << progOK << progOKArgs << false << true << true << outputQuiet;
	QTest::newRow("success") << progOK << progOKArgs << true << false << true << outputOK;
	QTest::newRow("success-background") << progOK << progOKArgs << true << true << true << outputOK;

	QTest::newRow("invalid-quiet") << progInvalid << progInvalidArgs << false << false << false << outputQuiet;
	QTest::newRow("invalid-quiet-background") << progInvalid << progInvalidArgs << false << true << false << outputQuiet;
	QTest::newRow("invalid") << progInvalid << progInvalidArgs << true << false << false << outputInvalid;
	QTest::newRow("invalid-background") << progInvalid << progInvalidArgs << true << true << false << outputInvalid;
}

void TestUtils::SystemCommand_getResult()
{
	QFETCH(QString, program);
	QFETCH(QStringList, args);
	QFETCH(bool, outputWanted);
	QFETCH(bool, runInBackground);
	QFETCH(bool, success);
	QFETCH(QString, output);

	Tw::Utils::SystemCommand * cmd = new Tw::Utils::SystemCommand(this, outputWanted, runInBackground);
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(cmd, SIGNAL(destroyed()));
#else
	QSignalSpy spy(cmd, &Tw::Utils::SystemCommand::destroyed);
#endif

	QVERIFY(spy.isValid());

	cmd->start(program, args);
	QCOMPARE(cmd->waitForFinished(), success);
#ifdef Q_OS_WIN
	QCOMPARE(cmd->getResult().replace(QStringLiteral("\r\n"), QStringLiteral("\n")), output);
#else
	QCOMPARE(cmd->getResult(), output);
#endif

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
		Tw::Utils::CommandlineParser clp(QStringList({exe,
			QStringLiteral("--oLong=asdf"),
			QStringLiteral("-o=ghjk"),
			QStringLiteral("qwerty"),
			QStringLiteral("-s"),
			QStringLiteral("--sLong"),
			QStringLiteral("--oLong"), QStringLiteral("xcvb"),
			QStringLiteral("-o"), QStringLiteral("vbnm"),
			QStringLiteral("uiop")}
		));
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
		int nextArg2 = clp.getNextArgument(nextArg);
		QCOMPARE(nextArg2, 7);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextArg2);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Argument);
			QCOMPARE(item.longName, QString());
			QCOMPARE(item.value, QVariant(QStringLiteral("uiop")));
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
		int nextOpt3 = clp.getNextOption(QStringLiteral("oLong"), nextOpt2);
		QCOMPARE(nextOpt3, 5);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextOpt3);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Option);
			QCOMPARE(item.longName, QStringLiteral("oLong"));
			QCOMPARE(item.value, QVariant(QStringLiteral("xcvb")));
			QVERIFY(item.processed == false);
		}
		int nextOpt4 = clp.getNextOption(QStringLiteral("oLong"), nextOpt3);
		QCOMPARE(nextOpt4, 6);
		{
			const Tw::Utils::CommandlineParser::CommandlineItem & item = clp.at(nextOpt4);
			QCOMPARE(item.type, Tw::Utils::CommandlineParser::Commandline_Option);
			QCOMPARE(item.longName, QStringLiteral("oLong"));
			QCOMPARE(item.value, QVariant(QStringLiteral("vbnm")));
			QVERIFY(item.processed == false);
		}
		QCOMPARE(clp.getNextOption(QStringLiteral("oLong"), nextOpt4), -1);

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
	QVERIFY(c != nullptr);

	QCOMPARE(c->mibEnum(), -4000);
	QCOMPARE(c->name(), QByteArray("Mac Central European Roman"));
	QCOMPARE(c->aliases(), QList<QByteArray>({"MacCentralEuropeanRoman", "MacCentralEurRoman"}));

	// € cannot be encoded and is replaced by ? (or \x00 if
	// QTextCodec::ConvertInvalidToNull is specified)
	QCOMPARE(c->fromUnicode(QStringLiteral("AÄĀ°§€")), QByteArray("\x41\x80\x81\xA1\xA4?"));
	QCOMPARE(c->toUnicode(QByteArray("\x41\x80\x81\xA1\xA4")), QStringLiteral("AÄĀ°§"));

	QCOMPARE(c->toUnicode(nullptr), QString());

	QTextEncoder * e = c->makeEncoder(QTextCodec::ConvertInvalidToNull);
	QVERIFY(e != nullptr);
	QCOMPARE(e->fromUnicode(QStringLiteral("AÄĀ°§€")), QByteArray("\x41\x80\x81\xA1\xA4\x00", 6));
	delete e;

	QTextDecoder * d = c->makeDecoder(QTextCodec::ConvertInvalidToNull);
	QVERIFY(d != nullptr);
	QCOMPARE(d->toUnicode(QByteArray("\x41\x80\x81\xA1\xA4")), QStringLiteral("AÄĀ°§"));
	delete d;
}

void TestUtils::FullscreenManager()
{
	{
		::UnitTest::FullscreenManager m(nullptr);
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
		QSignalSpy spy(&m, SIGNAL(fullscreenChanged(bool)));
#else
		QSignalSpy spy(&m, &::UnitTest::FullscreenManager::fullscreenChanged);
#endif
		QVERIFY(spy.isValid());
		QCOMPARE(m.isFullscreen(), false);
		m.toggleFullscreen();
		QCOMPARE(m.isFullscreen(), false);
		m.setFullscreen();
		QCOMPARE(m.isFullscreen(), false);
		QCOMPARE(spy.count(), 0);

		QCOMPARE(m.shortcuts().count(), 0);
		m.addShortcut(QKeySequence(Qt::Key_F), SLOT(update()));
		QCOMPARE(m.shortcuts().count(), 0);

		const QPoint mousePos{0, 0};
		QMouseEvent e(QMouseEvent::Move, mousePos, mousePos, Qt::NoButton, Qt::NoButton, {});
		m.mouseMoveEvent(&e);
	}
	{
		QMainWindow w;
		::UnitTest::FullscreenManager m(&w);
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
		QSignalSpy spy(&m, SIGNAL(fullscreenChanged(bool)));
#else
		QSignalSpy spy(&m, &::UnitTest::FullscreenManager::fullscreenChanged);
#endif
		QVERIFY(spy.isValid());

		w.setAttribute(Qt::WA_TranslucentBackground);
		w.setMenuBar(new QMenuBar);
		w.setStatusBar(new QStatusBar);
		QToolBar * tb = w.addToolBar(QString());

		w.show();
		QCoreApplication::processEvents();
		// Ensure the window is properly shown/activated/polished
		spy.wait(m.timeout());

		QCOMPARE(m.isFullscreen(), false);

		if (!w.menuBar()->isNativeMenuBar()) {
			QCOMPARE(w.menuBar()->isVisibleTo(&w), true);
		}
		QCOMPARE(w.statusBar()->isVisibleTo(&w), true);
		QCOMPARE(tb->isVisibleTo(&w), true);

		m.toggleFullscreen();

		QCOMPARE(m.isFullscreen(), true);
		if (!w.menuBar()->isNativeMenuBar()) {
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
		}
		QCOMPARE(w.statusBar()->isVisibleTo(&w), false);
		QCOMPARE(tb->isVisibleTo(&w), false);
		QCOMPARE(spy.count(), 1);
		QCOMPARE(spy[0][0].toBool(), true);

		if (!w.menuBar()->isNativeMenuBar()) {
			constexpr int threshold = 10;
			const QPoint overPos{0, threshold};
			const QPoint outPos{0, qMax(threshold, w.menuBar()->height()) + 1};
			QMouseEvent mouseOver(QMouseEvent::Move, overPos, overPos, Qt::NoButton, Qt::NoButton, {});
			QMouseEvent mouseOut(QMouseEvent::Move, outPos, outPos, Qt::NoButton, Qt::NoButton, {});

			// Hover over and move away quickly (should not trigger the menubar)
			m.mouseMoveEvent(&mouseOver);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
			spy.wait(m.timeout() / 2);
			m.mouseMoveEvent(&mouseOut);
			spy.wait(m.timeout() / 2);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
			spy.wait(m.timeout() / 2);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);

			// Hover over and wait
			m.mouseMoveEvent(&mouseOver);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
			spy.wait(m.timeout() + m.timeout() / 2);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), true);
			m.mouseMoveEvent(&mouseOut);
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
		}

		// Should do nothing - fullscreen is already active
		m.setFullscreen(true);

		QCOMPARE(m.isFullscreen(), true);
		if (!w.menuBar()->isNativeMenuBar()) {
			QCOMPARE(w.menuBar()->isVisibleTo(&w), false);
		}
		QCOMPARE(w.statusBar()->isVisibleTo(&w), false);
		QCOMPARE(tb->isVisibleTo(&w), false);
		QCOMPARE(spy.count(), 1);
		QCOMPARE(spy[0][0].toBool(), true);

		m.setFullscreen(false);

		QCOMPARE(m.isFullscreen(), false);
		if (!w.menuBar()->isNativeMenuBar()) {
			QCOMPARE(w.menuBar()->isVisibleTo(&w), true);
		}
		QCOMPARE(w.statusBar()->isVisibleTo(&w), true);
		QCOMPARE(tb->isVisibleTo(&w), true);
		QCOMPARE(spy.count(), 2);
		QCOMPARE(spy[1][0].toBool(), false);

		m.setFullscreen();
		QTest::keyClick(&w, Qt::Key_Escape);
		QCOMPARE(m.isFullscreen(), false);

		QCOMPARE(m.shortcuts().count(), 1);
		// Older versions of Qt don't support QCOMPARE of pointer and nullptr
		QVERIFY(m.shortcuts()[0].action == nullptr);
		QVERIFY(m.shortcuts()[0].shortcut != nullptr);
		QCOMPARE(m.shortcuts()[0].shortcut->key(), QKeySequence(Qt::Key_Escape));
		QCOMPARE(m.shortcuts()[0].shortcut->isEnabled(), false);

		{
			QAction * a = w.menuBar()->addAction(QStringLiteral("a"));
			m.addShortcut(a, SLOT(update()));
			m.addShortcut(QKeySequence(Qt::Key_F), SLOT(update()));

			QCOMPARE(m.shortcuts().count(), 3);
			QCOMPARE(m.shortcuts()[1].action, a);
			QVERIFY(m.shortcuts()[1].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[1].shortcut->key(), QKeySequence());
			QCOMPARE(m.shortcuts()[1].shortcut->isEnabled(), false);

			QVERIFY(m.shortcuts()[2].action == nullptr);
			QVERIFY(m.shortcuts()[2].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[2].shortcut->key(), QKeySequence(Qt::Key_F));
			QCOMPARE(m.shortcuts()[2].shortcut->isEnabled(), false);

			m.setFullscreen(true);

			QCOMPARE(m.shortcuts().count(), 3);
			QVERIFY(m.shortcuts()[0].action == nullptr);
			QVERIFY(m.shortcuts()[0].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[0].shortcut->key(), QKeySequence(Qt::Key_Escape));
			QCOMPARE(m.shortcuts()[0].shortcut->isEnabled(), true);

			QCOMPARE(m.shortcuts()[1].action, a);
			QVERIFY(m.shortcuts()[1].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[1].shortcut->key(), QKeySequence());
			if (!w.menuBar()->isNativeMenuBar()) {
				QCOMPARE(m.shortcuts()[1].shortcut->isEnabled(), true);
			}

			QVERIFY(m.shortcuts()[2].action == nullptr);
			QVERIFY(m.shortcuts()[2].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[2].shortcut->key(), QKeySequence(Qt::Key_F));
			QCOMPARE(m.shortcuts()[2].shortcut->isEnabled(), true);

			m.setFullscreen(false);

			QCOMPARE(m.shortcuts().count(), 3);
			QVERIFY(m.shortcuts()[0].action == nullptr);
			QVERIFY(m.shortcuts()[0].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[0].shortcut->key(), QKeySequence(Qt::Key_Escape));
			QCOMPARE(m.shortcuts()[0].shortcut->isEnabled(), false);

			QCOMPARE(m.shortcuts()[1].action, a);
			QVERIFY(m.shortcuts()[1].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[1].shortcut->key(), QKeySequence());
			if (!w.menuBar()->isNativeMenuBar()) {
				QCOMPARE(m.shortcuts()[1].shortcut->isEnabled(), false);
			}

			QVERIFY(m.shortcuts()[2].action == nullptr);
			QVERIFY(m.shortcuts()[2].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[2].shortcut->key(), QKeySequence(Qt::Key_F));
			QCOMPARE(m.shortcuts()[2].shortcut->isEnabled(), false);

			// Destroy the action to check if the corresponding shortcut is removed
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
			QSignalSpy deletionSpy(a, SIGNAL(destroyed(QObject*)));
#else
			QSignalSpy deletionSpy(a, &QAction::destroyed);
#endif
			a->deleteLater();
			QVERIFY(deletionSpy.wait());

			QCOMPARE(m.shortcuts().count(), 2);
			QVERIFY(m.shortcuts()[0].action == nullptr);
			QVERIFY(m.shortcuts()[0].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[0].shortcut->key(), QKeySequence(Qt::Key_Escape));
			QCOMPARE(m.shortcuts()[0].shortcut->isEnabled(), false);

			QVERIFY(m.shortcuts()[1].action == nullptr);
			QVERIFY(m.shortcuts()[1].shortcut != nullptr);
			QCOMPARE(m.shortcuts()[1].shortcut->key(), QKeySequence(Qt::Key_F));
			QCOMPARE(m.shortcuts()[1].shortcut->isEnabled(), false);
		}
	}
}

void TestUtils::ResourcesLibrary_getLibraryPath_data()
{
	QTest::addColumn<QString>("portableLibPath");
	QTest::addColumn<QString>("subdir");
	QTest::addColumn<QString>("result");

	const QString sConfig(QStringLiteral("configuration"));
	const QString sDicts(QStringLiteral("dictionaries"));
	const QString sInvalid(QStringLiteral("does-not-exist"));

#if QT_VERSION >= QT_VERSION_CHECK(5, 4, 0)
	QString stem = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + QStringLiteral("/");
#else
	QString stem = QStandardPaths::writableLocation(QStandardPaths::DataLocation) + QStringLiteral("/");
#endif

	QTest::newRow("root") << QString() << QString() << stem;
	QTest::newRow("configuration") << QString() << sConfig << stem + sConfig;
	QTest::newRow("does-not-exist") << QString() << sInvalid << stem + sInvalid;
#if defined(Q_OS_UNIX) && !defined(Q_OS_DARWIN) // *nix
#	ifndef TW_DICPATH
	QTest::newRow("dictionaries") << QString() << sDicts << QStringLiteral("/usr/share/hunspell:/usr/share/myspell/dicts");
#	else
	QTest::newRow("dictionaries") << QString() << sDicts << TW_DICPATH;
#	endif
#else // not *nix
	QTest::newRow("dictionaries") << QString() << sDicts << stem + sDicts;
#endif

	stem = "/invented/portable/root/";
	QTest::newRow("portable-root") << stem << QString() << stem;
	QTest::newRow("portable-configuration") << stem << sConfig << stem + sConfig;
	QTest::newRow("portable-does-not-exist") << stem << sInvalid << stem + sInvalid;
	QTest::newRow("portable-dictionaries") << stem << sDicts << stem + sDicts;
}

void TestUtils::ResourcesLibrary_getLibraryPath()
{
	QFETCH(QString, portableLibPath);
	QFETCH(QString, subdir);
	QFETCH(QString, result);

	Tw::Utils::ResourcesLibrary::setPortableLibPath(portableLibPath);
	QCOMPARE(Tw::Utils::ResourcesLibrary::getLibraryPath(subdir, false), result);
}

void TestUtils::ResourcesLibrary_portableLibPath()
{
	QString noDir;
	QString curDir(QStringLiteral("."));
	QString invalidDir(QStringLiteral("/does-not-exist/"));

	Tw::Utils::ResourcesLibrary::setPortableLibPath(noDir);
	QCOMPARE(Tw::Utils::ResourcesLibrary::getPortableLibPath(), noDir);
	Tw::Utils::ResourcesLibrary::setPortableLibPath(curDir);
	QCOMPARE(Tw::Utils::ResourcesLibrary::getPortableLibPath(), curDir);
	Tw::Utils::ResourcesLibrary::setPortableLibPath(invalidDir);
	QCOMPARE(Tw::Utils::ResourcesLibrary::getPortableLibPath(), invalidDir);
}

void TestUtils::TypesetManager()
{
	Tw::Utils::TypesetManager tm;
	QString empty;
	QString fileA{QStringLiteral("a")};
	QString fileB{QStringLiteral("b")};
	QObject owner;
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy started(&tm, SIGNAL(typesettingStarted(QString)));
	QSignalSpy stopped(&tm, SIGNAL(typesettingStopped(QString)));
#else
	QSignalSpy started(&tm, &Tw::Utils::TypesetManager::typesettingStarted);
	QSignalSpy stopped(&tm, &Tw::Utils::TypesetManager::typesettingStopped);
#endif

	QVERIFY(started.isValid());
	QVERIFY(stopped.isValid());

	// 1) no process running
	QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(empty), false);
	QVERIFY(tm.getOwnerForRootFile(fileA) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileA), false);
	QVERIFY(tm.getOwnerForRootFile(fileB) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileB), false);

	// 2a) start process
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);
	QCOMPARE(tm.startTypesetting(fileA, &owner), true);
	QCOMPARE(started.count(), 1);
	QCOMPARE(started.takeFirst().at(0).toString(), fileA);
	QCOMPARE(stopped.count(), 0);

	// 2b) one process running
	QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(empty), false);
	QVERIFY(tm.getOwnerForRootFile(fileA) == &owner);
	QCOMPARE(tm.isFileBeingTypeset(fileA), true);
	QVERIFY(tm.getOwnerForRootFile(fileB) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileB), false);

	// 2c) Can't start again
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);
	QCOMPARE(tm.startTypesetting(fileA, &owner), false);
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);

	// 2d) Can't start with invalid file
	QCOMPARE(tm.startTypesetting(empty, &owner), false);
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);

	// 2e) Can't start with invalid window
	QCOMPARE(tm.startTypesetting(fileB, nullptr), false);
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);

	// 2f) still one process running
	QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(empty), false);
	QVERIFY(tm.getOwnerForRootFile(fileA) == &owner);
	QCOMPARE(tm.isFileBeingTypeset(fileA), true);
	QVERIFY(tm.getOwnerForRootFile(fileB) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileB), false);

	// 3a) Start second process but destroy window
	{
		QObject owner2;
		QCOMPARE(started.count(), 0);
		QCOMPARE(stopped.count(), 0);
		QCOMPARE(tm.startTypesetting(fileB, &owner2), true);
		QCOMPARE(started.count(), 1);
		QCOMPARE(started.takeFirst().at(0).toString(), fileB);
		QCOMPARE(stopped.count(), 0);

		// two processes running
		QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
		QCOMPARE(tm.isFileBeingTypeset(empty), false);
		QVERIFY(tm.getOwnerForRootFile(fileA) == &owner);
		QCOMPARE(tm.isFileBeingTypeset(fileA), true);
		QVERIFY(tm.getOwnerForRootFile(fileB) == &owner2);
		QCOMPARE(tm.isFileBeingTypeset(fileB), true);

		// 3b) destroying the typesetting window (at the end of the scope) should stop
		QCOMPARE(started.count(), 0);
		QCOMPARE(stopped.count(), 0);
	}
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 1);
	QCOMPARE(stopped.takeFirst().at(0).toString(), fileB);

	// 3c) one process running
	QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(empty), false);
	QVERIFY(tm.getOwnerForRootFile(fileA) == &owner);
	QCOMPARE(tm.isFileBeingTypeset(fileA), true);
	QVERIFY(tm.getOwnerForRootFile(fileB) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileB), false);

	// 4a) stop process
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 0);
	tm.stopTypesetting(&owner);
	QCOMPARE(started.count(), 0);
	QCOMPARE(stopped.count(), 1);
	QCOMPARE(stopped.takeFirst().at(0).toString(), fileA);

	// 3b) no process running
	QVERIFY(tm.getOwnerForRootFile(empty) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(empty), false);
	QVERIFY(tm.getOwnerForRootFile(fileA) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileA), false);
	QVERIFY(tm.getOwnerForRootFile(fileB) == nullptr);
	QCOMPARE(tm.isFileBeingTypeset(fileB), false);
}

#ifdef Q_OS_DARWIN
void TestUtils::OSVersionString()
{
	QString version = GetMacOSVersionString();
	QRegularExpression pattern(QStringLiteral("^Mac OS X (\\d+)\\.(\\d+)\\.(\\d+)$"));
	QVERIFY2(version.contains(pattern), qPrintable(version));
}
#endif // defined(Q_OS_DARWIN)

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestUtils)
