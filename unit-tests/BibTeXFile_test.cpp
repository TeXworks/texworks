#include "BibTeXFile_test.h"
#include "BibTeXFile.h"

void TestBibTeXFile::load()
{
  BibTeXFile b;
  QVERIFY(b.load("bibtex-1.bib"));
  QVERIFY(b.load("does-not-exist.bib") == false);
}

void TestBibTeXFile::numEntries()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.numEntries(), static_cast<unsigned int>(1));
}

void TestBibTeXFile::entry_type()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).type(), BibTeXFile::Entry::NORMAL);
}

void TestBibTeXFile::entry_typeString()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).typeString(), QString::fromLatin1("article"));
}

void TestBibTeXFile::entry_key()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).key(), QString::fromLatin1("a1"));
}

void TestBibTeXFile::entry_hasField()
{
  BibTeXFile b("bibtex-1.bib");
  QVERIFY(b.entry(0).hasField(QString::fromLatin1("title")));
  QVERIFY(b.entry(0).hasField(QString::fromLatin1("TITLE")));
  QVERIFY(b.entry(0).hasField(QString::fromLatin1("unknown-field")) == false);
}

void TestBibTeXFile::entry_value()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).value(QString::fromLatin1("author")), QString::fromLatin1("John Doe and Smith, Jane"));
  QCOMPARE(b.entry(0).value(QString::fromLatin1("AUTHOR")), QString::fromLatin1("John Doe and Smith, Jane"));
  QCOMPARE(b.entry(0).value(QString::fromLatin1("unknown-field")), QString());
}

void TestBibTeXFile::entry_title()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).title(), QString::fromUtf8("Some article with unicode characters ä€𝄞"));
}

void TestBibTeXFile::entry_author()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).author(), QString::fromLatin1("John Doe and Smith, Jane"));
}

void TestBibTeXFile::entry_year()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).year(), QString::fromLatin1("1900"));
}

void TestBibTeXFile::entry_howPublished()
{
  BibTeXFile b("bibtex-1.bib");
  QCOMPARE(b.entry(0).howPublished(), QString());
}

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN (QWindowsIntegrationPlugin);
#endif

QTEST_MAIN(TestBibTeXFile)
