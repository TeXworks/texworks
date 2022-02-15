/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2020-2022  Stefan Löffler

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

#include "Document_test.h"

#include "../modules/QtPDF/src/PDFBackend.h"
#include "TWSynchronizer.h"
#include "TeXHighlighter.h"
#include "document/Document.h"
#include "document/SpellChecker.h"
#include "document/TeXDocument.h"
#include "document/TextDocument.h"
#include "utils/ResourcesLibrary.h"

#include <QSignalSpy>
#include <limits>

#if WITH_POPPLERQT
#if POPPLER_HAS_RUNTIME_VERSION
#include <poppler-version.h>
#else
#include <poppler-config.h>
#endif
#endif

Q_DECLARE_METATYPE(QSharedPointer<TWSyncTeXSynchronizer>)
Q_DECLARE_METATYPE(TWSynchronizer::TeXSyncPoint)
Q_DECLARE_METATYPE(TWSynchronizer::PDFSyncPoint)
Q_DECLARE_METATYPE(TWSynchronizer::Resolution)

void NonblockingSyntaxHighlighter::setDocument(QTextDocument * doc) { Q_UNUSED(doc) }
void NonblockingSyntaxHighlighter::rehighlight() { }
void NonblockingSyntaxHighlighter::rehighlightBlock(const QTextBlock & block) { Q_UNUSED(block) }
void NonblockingSyntaxHighlighter::maybeRehighlightText(int position, int charsRemoved, int charsAdded) { Q_UNUSED(position) Q_UNUSED(charsRemoved) Q_UNUSED(charsAdded) }
void NonblockingSyntaxHighlighter::process() { }
void NonblockingSyntaxHighlighter::processWhenIdle() {}
TeXHighlighter::TeXHighlighter(Tw::Document::TeXDocument * parent) : NonblockingSyntaxHighlighter(parent) { }
void TeXHighlighter::highlightBlock(const QString &text) { Q_UNUSED(text) }

char * toString(const TWSyncTeXSynchronizer::TeXSyncPoint & p) {
	return QTest::toString(QStringLiteral("TeXSyncPoint(%0 @ %1, %2 - %3)").arg(p.filename).arg(p.line).arg(p.col).arg(p.col + p.len));
}

char * toString(const TWSyncTeXSynchronizer::PDFSyncPoint & p) {
	QString rectStr;
	QDebug(&rectStr) << qSetRealNumberPrecision(20) << p.rects;
	return QTest::toString(QStringLiteral("PDFSyncPoint(%0 @ %1, %2)").arg(p.filename).arg(p.page).arg(rectStr));
}

#if WITH_POPPLERQT

#if QT_VERSION < QT_VERSION_CHECK(5, 6, 0)
// QVersionNumber was introduced in 5.6.0; for compatibility with older versions
// this class provides a rudimentary implementation if necessary
class VersionNumber {
	QVector<int> m_segments;
public:
	VersionNumber() = default;
	VersionNumber(int maj, int min, int mic) { m_segments << maj << min << mic; }
	static VersionNumber fromString(const QString & string) {
		VersionNumber rv;
		Q_FOREACH(const QString & s, string.split(QChar('.'))) {
			bool ok;
			int i = s.toInt(&ok);
			if (!ok) return {};
			rv.m_segments.append(i);
		}
		return rv;
	}
	static int compare(const VersionNumber & v1, const VersionNumber & v2) {
		for (int i = 0; i < qMax(v1.m_segments.size(), v2.m_segments.size()); ++i) {
			int a = (i < v1.m_segments.size() ? v1.m_segments[i] : 0);
			int b = (i < v2.m_segments.size() ? v2.m_segments[i] : 0);
			if (a < b)
				return -1;
			else if (a > b)
				return 1;
		}
		return 0;
	}
	bool operator==(const VersionNumber & rhs) const { return compare(*this, rhs) == 0; }
	bool operator!=(const VersionNumber & rhs) const { return compare(*this, rhs) != 0; }
	bool operator<(const VersionNumber & rhs) const { return compare(*this, rhs) < 0; }
	bool operator<=(const VersionNumber & rhs) const { return compare(*this, rhs) <= 0; }
	bool operator>(const VersionNumber & rhs) const { return compare(*this, rhs) > 0; }
	bool operator>=(const VersionNumber & rhs) const { return compare(*this, rhs) >= 0; }
};
#else
using VersionNumber = QVersionNumber;
#endif

VersionNumber popplerBuildVersion() {
	return VersionNumber::fromString(POPPLER_VERSION);
}

VersionNumber popplerRuntimeVersion() {
#if POPPLER_HAS_RUNTIME_VERSION
	return {static_cast<int>(Poppler::Version::major()), static_cast<int>(Poppler::Version::minor()), static_cast<int>(Poppler::Version::micro())};
#else
	return popplerBuildVersion();
#endif
}
#endif // WITH_POPPLERQT


namespace Tw {
namespace Utils {
// Referenced in Tw::Document::SpellChecker
const QStringList ResourcesLibrary::getLibraryPaths(const QString & subdir, const bool updateOnDisk) { Q_UNUSED(subdir) Q_UNUSED(updateOnDisk) return QStringList(QDir::currentPath()); }
} // namespace Utils
} // namespace Tw

namespace Tw {
namespace Document {

bool operator==(const TextDocument::Tag & t1, const TextDocument::Tag & t2) {
	return (t1.cursor == t2.cursor && t1.level == t2.level && t1.text == t2.text);
}

} // namespace Document
} // namespace Tw

namespace UnitTest {

void TestDocument::isPDFfile_data()
{
	QTest::addColumn<bool>("success");

	QTest::newRow("does-not-exist") << false;
	QTest::newRow("sync.tex") << false;
	QTest::newRow("base14-fonts.png") << false;
	QTest::newRow("base14-fonts.pdf") << true;
	QTest::newRow("hello-world.ps") << false;
}

void TestDocument::isPDFfile()
{
	QFETCH(bool, success);
	QCOMPARE(Tw::Document::isPDFfile(QString::fromUtf8(QTest::currentDataTag())), success);
}

void TestDocument::isImageFile_data()
{
	QTest::addColumn<bool>("success");

	QTest::newRow("does-not-exist") << false;
	QTest::newRow("sync.tex") << false;
	QTest::newRow("base14-fonts.png") << true;
	// Don't test .pdf and .ps files for now. They may or may not be recognized
	// depending on the OS, Qt version, plugins, etc.
//	QTest::newRow("base14-fonts.pdf") << false;
//	QTest::newRow("hello-world.ps") << false;
}

void TestDocument::isImageFile()
{
	QFETCH(bool, success);
	QCOMPARE(Tw::Document::isImageFile(QString::fromUtf8(QTest::currentDataTag())), success);
}

void TestDocument::isPostscriptFile_data()
{
	QTest::addColumn<bool>("success");

	QTest::newRow("does-not-exist") << false;
	QTest::newRow("sync.tex") << false;
	QTest::newRow("base14-fonts.png") << false;
	QTest::newRow("base14-fonts.pdf") << false;
	QTest::newRow("hello-world.ps") << true;
}

void TestDocument::isPostscriptFile()
{
	QFETCH(bool, success);
	QCOMPARE(Tw::Document::isPostscriptFile(QString::fromUtf8(QTest::currentDataTag())), success);
}

void TestDocument::fileInfo()
{
	Tw::Document::TextDocument doc;
	QFileInfo fi(QStringLiteral("base14-fonts.pdf"));

	QVERIFY(doc.getFileInfo().filePath().isEmpty());
	doc.setFileInfo(fi);
	QCOMPARE(doc.getFileInfo(), fi);
}

void TestDocument::storedInFilesystem()
{
	Tw::Document::TextDocument doc;
	QCOMPARE(doc.isStoredInFilesystem(), false);
	doc.setStoredInFilesystem(true);
	QCOMPARE(doc.isStoredInFilesystem(), true);
}

void TestDocument::absoluteFilePath()
{
	Tw::Document::TextDocument doc;
	QFileInfo fi(QStringLiteral("base14-fonts.pdf"));

	QCOMPARE(doc.absoluteFilePath(), QString());
	doc.setFileInfo(fi);
	QCOMPARE(doc.absoluteFilePath(), fi.absoluteFilePath());
}

void TestDocument::tags()
{
	Tw::Document::TextDocument doc(QStringLiteral("Hello World"));
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&doc, SIGNAL(tagsChanged()));
#else
	QSignalSpy spy(&doc, &Tw::Document::TextDocument::tagsChanged);
#endif

	Tw::Document::TextDocument::Tag tag1{QTextCursor(&doc), 0, QStringLiteral("tag1")};
	tag1.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 2);

	Tw::Document::TextDocument::Tag tag2{QTextCursor(&doc), 0, QStringLiteral("tag2")};
	tag2.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 2);
	tag2.cursor.movePosition(QTextCursor::NextCharacter, QTextCursor::KeepAnchor, 1);

	QVERIFY(spy.isValid());
	QVERIFY(doc.getTags().isEmpty());
	doc.addTag(tag2.cursor, tag2.level, tag2.text);
	doc.addTag(tag1.cursor, tag1.level, tag1.text);
	QCOMPARE(spy.count(), 2);

	QList<Tw::Document::TextDocument::Tag> tags = doc.getTags();
	QCOMPARE(tags, QList<Tw::Document::TextDocument::Tag>() << tag1 << tag2);

	spy.clear();
	QCOMPARE(doc.removeTags(3, 5), 0u);
	QCOMPARE(spy.count(), 0);

	QCOMPARE(doc.removeTags(0, 1), 1u);
	QCOMPARE(spy.count(), 1);
}

void TestDocument::getHighlighter()
{
	Tw::Document::TeXDocument doc;
	// Older versions of Qt don't support QCOMPARE of pointer and nullptr
	QVERIFY(doc.getHighlighter() == nullptr);
	TeXHighlighter highlighter(&doc);
	QCOMPARE(doc.getHighlighter(), &highlighter);
}

void TestDocument::modelines()
{
	Tw::Document::TeXDocument doc(QStringLiteral("Lorem ipsum\n").repeated(200));
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(&doc, SIGNAL(modelinesChanged(QStringList, QStringList)));
#else
	QSignalSpy spy(&doc, &Tw::Document::TeXDocument::modelinesChanged);
#endif

	// Work around QTBUG-43695
	doc.documentLayout();

	QVERIFY(spy.isValid());

	QVERIFY(doc.getModeLines().isEmpty());
	QCOMPARE(doc.hasModeLine(QStringLiteral("does-not-exist")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("does-not-exist")), QString());

	QCOMPARE(spy.count(), 0);

	// Append past the PeekLength
	QTextCursor cur(&doc);
	cur.movePosition(QTextCursor::End);
	cur.insertText(QStringLiteral("%!TEX key=value\n"));

	QCOMPARE(spy.count(), 0);
	QVERIFY(doc.getModeLines().isEmpty());

	QStringList changed({QStringLiteral("key")});
	QStringList removed;
	QMap<QString, QString> modelines({{QStringLiteral("key"), QStringLiteral("value")}});

	// Insert at front
	cur.movePosition(QTextCursor::Start);
	cur.insertText(QStringLiteral("%!TEX key=value\n"));
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy[0][0].toStringList(), changed);
	QCOMPARE(spy[0][1].toStringList(), removed);
	QCOMPARE(doc.getModeLines(), modelines);
	QCOMPARE(doc.hasModeLine(QStringLiteral("does-not-exist")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("does-not-exist")), QString());
	QCOMPARE(doc.hasModeLine(QStringLiteral("key")), true);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("key")), QStringLiteral("value"));

	spy.clear();
	// Remove
	changed.swap(removed);
	cur.movePosition(QTextCursor::Start, QTextCursor::KeepAnchor);
	cur.removeSelectedText();
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy[0][0].toStringList(), changed);
	QCOMPARE(spy[0][1].toStringList(), removed);
	QVERIFY(doc.getModeLines().isEmpty());
	QCOMPARE(doc.hasModeLine(QStringLiteral("does-not-exist")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("does-not-exist")), QString());
	QCOMPARE(doc.hasModeLine(QStringLiteral("key")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("key")), QString());

	spy.clear();
	// Double set
	changed.swap(removed);
	modelines.insert(QStringLiteral("key"), QStringLiteral("value1"));
	cur.insertText(QStringLiteral("%!TEX key=value\n%^^A!TEX key=value1\n"));
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy[0][0].toStringList(), changed);
	QCOMPARE(spy[0][1].toStringList(), removed);
	QCOMPARE(doc.getModeLines(), modelines);
	QCOMPARE(doc.hasModeLine(QStringLiteral("does-not-exist")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("does-not-exist")), QString());
	QCOMPARE(doc.hasModeLine(QStringLiteral("key")), true);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("key")), QStringLiteral("value1"));

	spy.clear();
	// Partial Remove
	modelines.insert(QStringLiteral("key"), QStringLiteral("value"));
	cur.movePosition(QTextCursor::PreviousBlock, QTextCursor::KeepAnchor);
	cur.removeSelectedText();
	QCOMPARE(spy.count(), 1);
	QCOMPARE(spy[0][0].toStringList(), changed);
	QCOMPARE(spy[0][1].toStringList(), removed);
	QCOMPARE(doc.getModeLines(), modelines);
	QCOMPARE(doc.hasModeLine(QStringLiteral("does-not-exist")), false);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("does-not-exist")), QString());
	QCOMPARE(doc.hasModeLine(QStringLiteral("key")), true);
	QCOMPARE(doc.getModeLineValue(QStringLiteral("key")), QStringLiteral("value"));
}

void TestDocument::findNextWord_data()
{
	QTest::addColumn<QString>("text");
	QTest::addColumn<int>("expectedStart");
	QTest::addColumn<int>("expectedEnd");
	QTest::addColumn<bool>("returnValue");

	/*
a  testcase's word \command \comm@nd \cmd123 $ \[ öÄéàßÇα \@a'quote'
	 */
	QString s = QStringLiteral("a\t testcase's word \\command \\comm@nd \\cmd123 $ \\[ öÄéàßÇα \\@a'quote'");

	QTest::newRow("empty") << QString() << 42 << 42 << false;
	QTest::newRow("beyond-end") << s << 123 << 123 << false;
	QTest::newRow("single-char") << s << 0 << 1 << true;
	QTest::newRow("white-space") << s << 1 << 3 << false;
	QTest::newRow("word-with-apostrophe") << s << 3 << 13 << true;
	QTest::newRow("backslash") << s << 19 << 27 << false;
	QTest::newRow("command") << s << 19 << 27 << false;
	QTest::newRow("@") << s << 28 << 36 << false;
	QTest::newRow("@-command") << s << 28 << 36 << false;
	QTest::newRow("digit") << s << 41 << 44 << false;
	QTest::newRow("command-digit") << s << 37 << 41 << false;
	QTest::newRow("single-glyph") << s << 45 << 46 << false;
	QTest::newRow("command-glyph") << s << 47 << 49 << false;
	QTest::newRow("non-ascii") << s << 50 << 57 << true;
	QTest::newRow("command-apostrophe") << s << 58 << 61 << false;
}

void TestDocument::findNextWord()
{
	QFETCH(QString, text);
	QFETCH(int, expectedStart);
	QFETCH(int, expectedEnd);
	QFETCH(bool, returnValue);

	for (int index = expectedStart; index < qMax(expectedStart + 1, expectedEnd); ++index) {
		int start{std::numeric_limits<int>::min()}, end{std::numeric_limits<int>::min()};

		// Note that ' is currently not considered a word-forming character
		// See 33402c4, https://tug.org/pipermail/texworks/2009q2/000639.html,
		// https://tug.org/pipermail/texworks/2009q2/000642.html
		if (index == 11) {
			QEXPECT_FAIL("word-with-apostrophe", "The apostrophe by itself is not considered part of a word", Abort);
		}

		bool rv = Tw::Document::TeXDocument::findNextWord(text, index, start, end);

		QString msg = QStringLiteral("\"%1\" [expected: %2--%3, actual %4--%5, index %6]").arg(text).arg(expectedStart).arg(expectedEnd).arg(start).arg(end).arg(index);
		msg.insert(index + 1, QChar('|'));

		QVERIFY2(rv == returnValue, qPrintable(msg));
		QVERIFY2(start == expectedStart, qPrintable(msg));
		QVERIFY2(end == expectedEnd, qPrintable(msg));
	}
}

void TestDocument::SpellChecker_getDictionaryList()
{
	auto * sc = Tw::Document::SpellChecker::instance();
	Q_ASSERT(sc != nullptr);
#if QT_VERSION < QT_VERSION_CHECK(5, 4, 0)
	QSignalSpy spy(sc, SIGNAL(dictionaryListChanged()));
#else
	QSignalSpy spy(sc, &Tw::Document::SpellChecker::dictionaryListChanged);
#endif

	QVERIFY(spy.isValid());

	QCOMPARE(spy.count(), 0);

	auto dictList = sc->getDictionaryList();
	Q_ASSERT(dictList);

	QCOMPARE(spy.count(), 1);
	QVERIFY(dictList->contains(QDir::current().absoluteFilePath(QStringLiteral("dictionary.dic")), QStringLiteral("dictionary")));

	// Calling getDictionaryList() again (without forcing a reload) should give
	// the same data again
	QCOMPARE(sc->getDictionaryList(), dictList);
	QCOMPARE(spy.count(), 1);

	// Calling getDictionaryList() with forceReload should emit the
	// dictionaryListChanged signal again
	sc->getDictionaryList(true);
	QCOMPARE(spy.count(), 2);
}

void TestDocument::SpellChecker_getDictionary()
{
	QString lang{QStringLiteral("dictionary")};
	QString correctWord{QStringLiteral("World")};
	QString wrongWord{QStringLiteral("Wrld")};

	auto * sc = Tw::Document::SpellChecker::instance();
	Q_ASSERT(sc != nullptr);

	QVERIFY(sc->getDictionary(QString()) == nullptr);
	QVERIFY(sc->getDictionary(QStringLiteral("does-not-exist")) == nullptr);

	auto * d = sc->getDictionary(lang);
	QVERIFY(d != nullptr);
	QCOMPARE(sc->getDictionary(lang), d);

	QCOMPARE(d->getLanguage(), lang);
	QCOMPARE(d->isWordCorrect(correctWord), true);
	QCOMPARE(d->isWordCorrect(wrongWord), false);

	QCOMPARE(d->suggestionsForWord(wrongWord), QList<QString>{correctWord});
}

void TestDocument::SpellChecker_ignoreWord()
{
	QString lang{QStringLiteral("dictionary")};
	QString wrongWord{QStringLiteral("Wrld")};

	auto * sc = Tw::Document::SpellChecker::instance();
	Q_ASSERT(sc != nullptr);

	{
		auto * d = sc->getDictionary(lang);
		Q_ASSERT(d != nullptr);

		QCOMPARE(d->isWordCorrect(wrongWord), false);
		d->ignoreWord(wrongWord);
		QCOMPARE(d->isWordCorrect(wrongWord), true);
	}
	{
		// Check that ignoring is not persistent
		sc->clearDictionaries();

		auto * d = sc->getDictionary(lang);
		Q_ASSERT(d != nullptr);

		QCOMPARE(d->isWordCorrect(wrongWord), false);
	}
}

void TestDocument::Synchronizer_isValid()
{
	TWSyncTeXSynchronizer valid(QStringLiteral("sync.pdf"), nullptr, nullptr);
	TWSyncTeXSynchronizer invalid(QStringLiteral("does-not-exist"), nullptr, nullptr);

	QCOMPARE(valid.isValid(), true);
	QCOMPARE(invalid.isValid(), false);
}

void TestDocument::syncTeXFilename()
{
	TWSyncTeXSynchronizer valid(QStringLiteral("sync.pdf"), nullptr, nullptr);
	TWSyncTeXSynchronizer invalid(QStringLiteral("does-not-exist"), nullptr, nullptr);

	QCOMPARE(valid.syncTeXFilename(), QStringLiteral("sync.synctex.gz"));
	QCOMPARE(invalid.syncTeXFilename(), QString());
}

void TestDocument::pdfFilename()
{
	const QString pdfFilename(QStringLiteral("sync.pdf"));
	TWSyncTeXSynchronizer valid(pdfFilename, nullptr, nullptr);
	TWSyncTeXSynchronizer invalid(QStringLiteral("does-not-exist"), nullptr, nullptr);

	QCOMPARE(valid.pdfFilename(), pdfFilename);
	QCOMPARE(invalid.pdfFilename(), QString());
}

void TestDocument::Synchronizer_syncFromTeX_data()
{
	const QString texFilename(QStringLiteral("sync.tex"));
	const QString pdfFilename(QStringLiteral("sync.pdf"));

	QTest::addColumn<QSharedPointer<TWSyncTeXSynchronizer>>("synchronizer");
	QTest::addColumn<TWSynchronizer::Resolution>("resolution");
	QTest::addColumn<TWSynchronizer::TeXSyncPoint>("texPoint");
	QTest::addColumn<TWSynchronizer::PDFSyncPoint>("pdfPoint");

	QSharedPointer<QtPDF::Backend::Document> pdfDoc = QtPDF::Backend::Document::newDocument(pdfFilename);
	QFile f(texFilename);
	QVERIFY(f.open(QIODevice::ReadOnly));
	QTextStream strm(&f);
	QSharedPointer<Tw::Document::TeXDocument> texDoc(new Tw::Document::TeXDocument(strm.readAll()));

	QSharedPointer<TWSyncTeXSynchronizer> synchronizer{new TWSyncTeXSynchronizer(
		pdfFilename,
		[texDoc](const QString &) { return texDoc.data(); },
		[pdfDoc](const QString &) { return pdfDoc; })};

	// Line resolution checks
	QTest::newRow("simple start-of-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("simple space (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 5, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("simple end-of-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 11, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("empty-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 10, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("text-command (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, 3, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 151.756454467773, 343.711059570313, 6.918498039246)})});

	QTest::newRow("long-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, 16, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 163.711624145508, 365.353179931641, 8.855676651001)})});

	QTest::newRow("simple-footnote before (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 656.538208007813, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 186.43083190918, 343.711059570313, 8.109618186951)})});

	QTest::newRow("simple-footnote \\footnote (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 19, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 656.538208007813, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 186.43083190918, 343.711059570313, 8.109618186951)})});

	QTest::newRow("simple-footnote inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 24, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 656.538208007813, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 186.43083190918, 343.711059570313, 8.109618186951)})});

	QTest::newRow("complex-footnote before (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 666.042663574219, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 198.386001586914, 343.711059570313, 10.046796798706)})});

	QTest::newRow("complex-footnote \\footnote (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 48, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 666.042663574219, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 198.386001586914, 343.711059570313, 10.046796798706)})});

	QTest::newRow("complex-footnote inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 53, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 666.042663574219, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 198.386001586914, 343.711059570313, 10.046796798706)})});

	QTest::newRow("simple-section \\section (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 229.598388671875, 343.711059570313, 9.843077659607)})});

	QTest::newRow("simple-section inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 229.598388671875, 343.711059570313, 9.843077659607)})});

	QTest::newRow("complex-section \\section (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 257.27734375, 343.711059570313, 9.962624549866)})});

	QTest::newRow("complex-section inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 257.27734375, 343.7110595703125, 9.9626245498657)})});

	// Word resolution checks
	QTest::newRow("simple start-of-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 127.85092784, 22.41594, 8.84682432)})});

	QTest::newRow("simple space (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 5, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 127.85092784, 22.41594, 8.84682432)})});

	QTest::newRow("simple end-of-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 11, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(174.44549912, 127.85092784, 26.599252536, 8.84682432)})});

	QTest::newRow("empty-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 10, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("text-command (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, 3, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 151.756454467773, 343.711059570313, 6.918498039246)})});

	QTest::newRow("long-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, 16, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 163.71592784, 350.408962872, 8.84682432)})});

	QTest::newRow("simple-footnote before (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 187.62692784, 20.977334784, 8.84682432)})});

	QTest::newRow("simple-footnote \\footnote (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 19, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 656.538208007813, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 186.43083190918, 343.711059570313, 8.109618186951)})});

	QTest::newRow("simple-footnote inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 24, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(149.011, 657.67174366, 3.049364086, 7.07745768)})});

	QTest::newRow("complex-footnote before (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 199.58192784, 7.47198, 8.84682432)})});

	QTest::newRow("complex-footnote \\footnote (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 48, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 666.042663574219, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 198.386001586914, 343.711059570313, 10.046796798706)})});

	QTest::newRow("complex-footnote inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 53, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(149.011, 667.1767436599999, 6.342613537999995, 7.077457680000066)})});

	QTest::newRow("simple-section \\section (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 229.598388671875, 343.711059570313, 9.843077659607)})});

	QTest::newRow("simple-section inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(157.9772125, 229.4857372, 35.31747516, 12.7394256)})});

	QTest::newRow("complex-section \\section (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 257.27734375, 343.711059570313, 9.962624549866)})});

	QTest::newRow("complex-section inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(157.9772125, 257.2837372, 48.69961052, 12.7394256)})});

	// Character resolution checks
	QTest::newRow("simple start-of-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 127.85092784, 7.47198, 8.84682432)})});

	QTest::newRow("simple space (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 5, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 127.85092784, 22.41594, 8.84682432)})});

	QTest::newRow("simple end-of-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 11, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(195.509508872, 127.85092784, 5.535242784, 8.84682432)})});

	QTest::newRow("empty-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 10, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.7683563232422, 127.84612274169922, 343.7110595703125, 6.9184980392456055)})});

	QTest::newRow("text-command (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, 3, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 151.756454467773, 343.711059570313, 6.918498039246)})});

	QTest::newRow("long-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, 16, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(234.232298024, 163.71592784, 2.767621392, 8.84682432)})});

	QTest::newRow("simple-footnote before (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 187.62692784, 6.503611392, 8.84682432)})});

	QTest::newRow("simple-footnote \\footnote (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 19, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 656.538208007813, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 186.43083190918, 343.711059570313, 8.109618186951)})});

	QTest::newRow("simple-footnote inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 24, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(149.011, 657.67174366, 3.049364086, 7.07745768)})});

	QTest::newRow("complex-footnote before (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(148.712, 199.58192784, 7.47198, 8.84682432)})});

	QTest::newRow("complex-footnote \\footnote (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 48, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 666.042663574219, 343.711059570313, 9.504366874695), QRectF(133.768356323242, 198.386001586914, 343.711059570313, 10.046796798706)})});

	QTest::newRow("complex-footnote inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, 53, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(149.011, 667.1767436599999, 6.342613537999995, 7.077457680000066)})});

	QTest::newRow("simple-section \\section (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 229.598388671875, 343.711059570313, 9.843077659607)})});

	QTest::newRow("simple-section inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(157.9772125, 229.4857372, 12.1870969, 12.7394256)})});

	QTest::newRow("complex-section \\section (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 0, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(133.768356323242, 257.27734375, 343.711059570313, 9.962624549866)})});

	QTest::newRow("complex-section inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, 9, 0}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(157.9772125, 257.2837372, 6.36684356, 12.7394256)})});
}

void TestDocument::Synchronizer_syncFromTeX()
{
	QFETCH(QSharedPointer<TWSyncTeXSynchronizer>, synchronizer);
	QFETCH(TWSynchronizer::Resolution, resolution);
	QFETCH(TWSynchronizer::TeXSyncPoint, texPoint);
	QFETCH(TWSynchronizer::PDFSyncPoint, pdfPoint);

#if WITH_POPPLERQT
	if (QtPDF::Backend::Document::defaultBackend() == QStringLiteral("poppler-qt")) {
		QEXPECT_FAIL("complex-footnote inside (word)", "Complex footnotes don't always work yet", Continue);
		QEXPECT_FAIL("complex-footnote inside (char)", "Complex footnotes don't always work yet", Continue);
		if (popplerRuntimeVersion() >= VersionNumber(22, 1, 0)) {
			QEXPECT_FAIL("complex-footnote before (word)", "Complex footnotes don't always work yet (poppler >= 22.01.0)", Continue);
			QEXPECT_FAIL("complex-footnote before (char)", "Complex footnotes don't always work yet (poppler >= 22.01.0)", Continue);
		}
	}
#endif
	QCOMPARE(synchronizer->syncFromTeX(texPoint, resolution), pdfPoint);
}

void TestDocument::Synchronizer_syncFromPDF_data()
{
	const QString texFilename(QStringLiteral("./sync.tex"));
	const QString pdfFilename(QStringLiteral("sync.pdf"));

	QTest::addColumn<QSharedPointer<TWSyncTeXSynchronizer>>("synchronizer");
	QTest::addColumn<TWSynchronizer::Resolution>("resolution");
	QTest::addColumn<TWSynchronizer::TeXSyncPoint>("texPoint");
	QTest::addColumn<TWSynchronizer::PDFSyncPoint>("pdfPoint");

	QSharedPointer<QtPDF::Backend::Document> pdfDoc = QtPDF::Backend::Document::newDocument(pdfFilename);
	QFile f(texFilename);
	QVERIFY(f.open(QIODevice::ReadOnly));
	QTextStream strm(&f);
	QSharedPointer<Tw::Document::TeXDocument> texDoc(new Tw::Document::TeXDocument(strm.readAll()));

	QSharedPointer<TWSyncTeXSynchronizer> synchronizer{new TWSyncTeXSynchronizer(
		pdfFilename,
		[texDoc](const QString &) { return texDoc.data(); },
		[pdfDoc](const QString &) { return pdfDoc; })};

	// Line resolution checks
	QTest::newRow("top left (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(50, 50, 0, 0)})});

	QTest::newRow("simple start-of-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 130, 0, 0)})});

	QTest::newRow("simple space (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(173, 130, 0, 0)})});

	QTest::newRow("simple end-of-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(197, 130, 0, 0)})});

	QTest::newRow("text-command (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(167, 155, 0, 0)})});

	QTest::newRow("long-line (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(235, 165, 0, 0)})});

	QTest::newRow("simple-footnote before (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 190, 0, 0)})});

	QTest::newRow("simple-footnote inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 660, 0, 0)})});

	QTest::newRow("complex-footnote before (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 205, 0, 0)})});

	QTest::newRow("complex-footnote inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 670, 0, 0)})});

	QTest::newRow("simple-section inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 235, 0, 0)})});

	QTest::newRow("complex-section inside (line)") << synchronizer << TWSynchronizer::LineResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 260, 0, 0)})});

	// Word resolution checks
	QTest::newRow("top left (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(50, 50, 0, 0)})});

	QTest::newRow("simple start-of-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 0, 5}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 130, 0, 0)})});

	QTest::newRow("simple space (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(173, 130, 0, 0)})});

	QTest::newRow("simple end-of-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 6, 5}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(197, 130, 0, 0)})});

	QTest::newRow("text-command (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(167, 155, 0, 0)})});

	QTest::newRow("long-line (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, 0, 85}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(235, 165, 0, 0)})});

	QTest::newRow("simple-footnote before (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 0, 5}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 190, 0, 0)})});

	QTest::newRow("simple-footnote inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 24, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 660, 0, 0)})});

	QTest::newRow("complex-footnote before (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 205, 0, 0)})});

	QTest::newRow("complex-footnote inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 670, 0, 0)})});

	QTest::newRow("simple-section inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 9, 3}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 235, 0, 0)})});

	QTest::newRow("complex-section inside (word)") << synchronizer << TWSynchronizer::WordResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 260, 0, 0)})});

	// Character resolution checks
	QTest::newRow("top left (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(50, 50, 0, 0)})});

	QTest::newRow("simple start-of-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 0, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 130, 0, 0)})});

	QTest::newRow("simple space (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(173, 130, 0, 0)})});

	QTest::newRow("simple end-of-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 9, 10, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(197, 130, 0, 0)})});

	QTest::newRow("text-command (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 13, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(167, 155, 0, 0)})});

	QTest::newRow("long-line (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 15, 16, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(235, 165, 0, 0)})});

	QTest::newRow("simple-footnote before (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 0, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 190, 0, 0)})});

	QTest::newRow("simple-footnote inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 17, 24, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 660, 0, 0)})});

	QTest::newRow("complex-footnote before (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 205, 0, 0)})});

	QTest::newRow("complex-footnote inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 19, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(150, 670, 0, 0)})});

	QTest::newRow("simple-section inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 21, 9, 1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 235, 0, 0)})});

	QTest::newRow("complex-section inside (char)") << synchronizer << TWSynchronizer::CharacterResolution <<
		TWSynchronizer::TeXSyncPoint({texFilename, 23, -1, -1}) <<
		TWSynchronizer::PDFSyncPoint({pdfFilename, 1, QList<QRectF>({QRectF(160, 260, 0, 0)})});
}

void TestDocument::Synchronizer_syncFromPDF()
{
	QFETCH(QSharedPointer<TWSyncTeXSynchronizer>, synchronizer);
	QFETCH(TWSynchronizer::Resolution, resolution);
	QFETCH(TWSynchronizer::TeXSyncPoint, texPoint);
	QFETCH(TWSynchronizer::PDFSyncPoint, pdfPoint);

	QCOMPARE(synchronizer->syncFromPDF(pdfPoint, resolution), texPoint);
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestDocument)
