#include "TestQtPDF.h"

QTestData & TestQtPDF::newDocTest(const char * tag)
{
  return QTest::newRow(tag) << _docs[QString::fromUtf8(tag)];
}

void TestQtPDF::loadDocs()
{
#ifdef USE_MUPDF
  QtPDF::MuPDFBackend backend;
#elif USE_POPPLERQT
  QtPDF::PopplerQtBackend backend;
#else
  #error Must specify one backend
#endif

  // Don't run documents that may produce error messages in QBENCHMARK as
  // otherwise the console may be filled with unhelpful messages
  _docs[QString::fromAscii("invalid")] = backend.newDocument(QString());
  _docs[QString::fromAscii("base14-locked")] = backend.newDocument(QString::fromAscii("base14-fonts-locked.pdf"));

  QBENCHMARK {
    _docs[QString::fromAscii("transitions")] = backend.newDocument(QString::fromAscii("pdf-transitions.pdf"));
    _docs[QString::fromAscii("pgfmanual")] = backend.newDocument(QString::fromAscii("pgfmanual.pdf"));
    _docs[QString::fromAscii("base14-fonts")] = backend.newDocument(QString::fromAscii("base14-fonts.pdf"));
    _docs[QString::fromAscii("metadata")] = backend.newDocument(QString::fromAscii("metadata.pdf"));
  }
}

void TestQtPDF::isValid_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<bool>("expected");
  newDocTest("invalid") << false;
  newDocTest("transitions") << true;
  newDocTest("pgfmanual") << true;
  newDocTest("base14-fonts") << true;
  newDocTest("base14-locked") << true;
  newDocTest("metadata") << true;
}

void TestQtPDF::isValid()
{
  QFETCH(pDoc, doc);
  QFETCH(bool, expected);
  QCOMPARE(doc->isValid(), expected);
}

void TestQtPDF::isLocked_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<bool>("expected");
  newDocTest("invalid") << false;
  newDocTest("transitions") << false;
  newDocTest("pgfmanual") << false;
  newDocTest("base14-fonts") << false;
  newDocTest("base14-locked") << true;
  newDocTest("metadata") << false;
}

void TestQtPDF::isLocked()
{
  QFETCH(pDoc, doc);
  QFETCH(bool, expected);
  QCOMPARE(doc->isLocked(), expected);
}

void TestQtPDF::numPages_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<int>("expected");
  newDocTest("invalid") << -1;
  newDocTest("transitions") << 29;
  newDocTest("pgfmanual") << 726;
  newDocTest("base14-fonts") << 1;
  newDocTest("base14-locked") << 1;
  newDocTest("metadata") << 1;
}

void TestQtPDF::numPages()
{
  QFETCH(pDoc, doc);
  QFETCH(int, expected);
#ifdef USE_POPPLERQT
  QEXPECT_FAIL("base14-locked", "poppler-qt doesn't report page numbers for locked documents", Continue);
#endif
  QCOMPARE(doc->numPages(), expected);
}

void TestQtPDF::fileName_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString::fromAscii("pdf-transitions.pdf");
  newDocTest("pgfmanual") << QString::fromAscii("pgfmanual.pdf");
  newDocTest("base14-fonts") << QString::fromAscii("base14-fonts.pdf");
  newDocTest("base14-locked") << QString::fromAscii("base14-fonts-locked.pdf");
  newDocTest("metadata") << QString::fromAscii("metadata.pdf");
}

void TestQtPDF::fileName()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->fileName(), expected);
}

void TestQtPDF::page_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QSizeF>("pageSize");

  newDocTest("invalid") << QSizeF();
  newDocTest("transitions") << QSizeF(362.835, 272.126);
  newDocTest("pgfmanual") << QSizeF(595.276, 841.89);
  newDocTest("base14-fonts") << QSizeF(595, 842);
  newDocTest("base14-locked") << QSizeF(595, 842);
  newDocTest("metadata") << QSizeF(612, 792);
}

void TestQtPDF::page()
{
  QFETCH(pDoc, doc);
  QFETCH(QSizeF, pageSize);

  QVERIFY(doc->page(-1).isNull());
  QVERIFY(doc->page(doc->numPages()).isNull());

  for (int i = 0; i < doc->numPages(); ++i) {
    QSharedPointer<QtPDF::Backend::Page> page = doc->page(i).toStrongRef();

    QVERIFY(!page.isNull());
    QVERIFY(page->pageNum() == i);
#ifdef USE_POPPLERQT
    QEXPECT_FAIL("base14-locked", "poppler-qt doesn't report page sizes for locked documents", Continue);
#endif
    QVERIFY(qAbs(page->pageSizeF().width() - pageSize.width()) < 1e-4);
    QVERIFY(qAbs(page->pageSizeF().height() - pageSize.height()) < 1e-4);

//		transition
//		loadLinks()
//		boxes()
//		selectedText
//		renderToImage
//		loadAnnotations
//		search
  }
}


void TestQtPDF::metaDataTitle_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString();
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromUtf8("Document Title • UTF16-BE");
}

void TestQtPDF::metaDataTitle()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->title(), expected);
}

void TestQtPDF::metaDataAuthor_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString();
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromUtf8("Stefan Löffler");
}

void TestQtPDF::metaDataAuthor()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->author(), expected);
}

void TestQtPDF::metaDataSubject_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString();
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromAscii("PDF Test File");
}

void TestQtPDF::metaDataSubject()
{
    QFETCH(pDoc, doc);
    QFETCH(QString, expected);
    QCOMPARE(doc->subject(), expected);
}

void TestQtPDF::metaDataKeywords_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString();
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromAscii("pdf, metadata, test");
}

void TestQtPDF::metaDataKeywords()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->keywords(), expected);
}

void TestQtPDF::metaDataCreator_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString::fromUtf8("LaTeX with hyperref package");
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromAscii("gedit");
}

void TestQtPDF::metaDataCreator()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->creator(), expected);
}

void TestQtPDF::metaDataProducer_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QString>("expected");
  newDocTest("invalid") << QString();
  newDocTest("transitions") << QString();
  newDocTest("pgfmanual") << QString::fromUtf8("pdfTeX-1.40.10");
  newDocTest("base14-fonts") << QString();
  newDocTest("base14-locked") << QString();
  newDocTest("metadata") << QString::fromAscii("also gedit");
}

void TestQtPDF::metaDataProducer()
{
  QFETCH(pDoc, doc);
  QFETCH(QString, expected);
  QCOMPARE(doc->producer(), expected);
}

void TestQtPDF::metaDataCreationDate_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QDateTime>("expected");
  newDocTest("invalid") << QDateTime();
  newDocTest("transitions") << QDateTime();
  newDocTest("pgfmanual") << QDateTime(QDate(2010, 10, 25), QTime(22, 56, 26));
  newDocTest("base14-fonts") << QDateTime();
  newDocTest("base14-locked") << QDateTime();
  newDocTest("metadata") << QDateTime(QDate(2013, 9, 8), QTime(1, 23, 45));
}

void TestQtPDF::metaDataCreationDate()
{
  QFETCH(pDoc, doc);
  QFETCH(QDateTime, expected);
  QCOMPARE(doc->creationDate(), expected);
}

void TestQtPDF::metaDataModDate_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QDateTime>("expected");
  newDocTest("invalid") << QDateTime();
  newDocTest("transitions") << QDateTime();
  newDocTest("pgfmanual") << QDateTime(QDate(2010, 10, 25), QTime(22, 56, 26));
  newDocTest("base14-fonts") << QDateTime();
  newDocTest("base14-locked") << QDateTime();
  newDocTest("metadata") << QDateTime(QDate(2013, 9, 8), QTime(12, 34, 56));
}

void TestQtPDF::metaDataModDate()
{
  QFETCH(pDoc, doc);
  QFETCH(QDateTime, expected);
  QCOMPARE(doc->modDate(), expected);
}

void TestQtPDF::metaDataTrapped_data()
{
    QTest::addColumn<pDoc>("doc");
    QTest::addColumn<int>("expected");
    newDocTest("invalid") << (int)QtPDF::Backend::Document::Trapped_Unknown;
    newDocTest("transitions") << (int)QtPDF::Backend::Document::Trapped_Unknown;
    newDocTest("pgfmanual") << (int)QtPDF::Backend::Document::Trapped_False;
    newDocTest("base14-fonts") << (int)QtPDF::Backend::Document::Trapped_Unknown;
    newDocTest("base14-locked") << (int)QtPDF::Backend::Document::Trapped_Unknown;
    newDocTest("metadata") << (int)QtPDF::Backend::Document::Trapped_Unknown;
}

void TestQtPDF::metaDataTrapped()
{
    QFETCH(pDoc, doc);
    QFETCH(int, expected);
#ifdef USE_POPPLERQT
    QEXPECT_FAIL("pgfmanual", "poppler-qt doesn't handle trapping properly", Continue);
#endif
    QCOMPARE((int)doc->trapped(), expected);
}










QTEST_MAIN(TestQtPDF)
//#include "TestQtPDF.moc"
