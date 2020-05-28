/*
	This is part of TeXworks, an environment for working with TeX documents
	Copyright (C) 2020  Stefan Löffler

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

#include "TeXHighlighter.h"
#include "document/Document.h"
#include "document/TeXDocument.h"
#include "document/TextDocument.h"

#include <QSignalSpy>

void NonblockingSyntaxHighlighter::setDocument(QTextDocument * doc) { Q_UNUSED(doc) }
void NonblockingSyntaxHighlighter::rehighlight() { }
void NonblockingSyntaxHighlighter::rehighlightBlock(const QTextBlock & block) { Q_UNUSED(block) }
void NonblockingSyntaxHighlighter::maybeRehighlightText(int position, int charsRemoved, int charsAdded) { Q_UNUSED(position) Q_UNUSED(charsRemoved) Q_UNUSED(charsAdded) }
void NonblockingSyntaxHighlighter::process() { }
void NonblockingSyntaxHighlighter::processWhenIdle() {}
TeXHighlighter::TeXHighlighter(Tw::Document::TeXDocument * parent) : NonblockingSyntaxHighlighter(parent) { }
void TeXHighlighter::highlightBlock(const QString &text) { Q_UNUSED(text) }

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
	QTest::newRow("base14-fonts.png") << true;
	QTest::newRow("base14-fonts.pdf") << false;
	QTest::newRow("hello-world.ps") << false;
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
	QSignalSpy spy(&doc, SIGNAL(tagsChanged()));

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
	QSignalSpy spy(&doc, SIGNAL(modelinesChanged(QStringList, QStringList)));

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
	cur.insertText(QStringLiteral("%!TEX key=value\n%!TEX key=value1\n"));
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

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestDocument)
