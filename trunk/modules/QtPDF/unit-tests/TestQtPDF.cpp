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
  _docs[QString::fromLatin1("invalid")] = backend.newDocument(QString());
  _docs[QString::fromLatin1("base14-locked")] = backend.newDocument(QString::fromLatin1("base14-fonts-locked.pdf"));

  QBENCHMARK {
    _docs[QString::fromLatin1("transitions")] = backend.newDocument(QString::fromLatin1("pdf-transitions.pdf"));
    _docs[QString::fromLatin1("pgfmanual")] = backend.newDocument(QString::fromLatin1("pgfmanual.pdf"));
    _docs[QString::fromLatin1("base14-fonts")] = backend.newDocument(QString::fromLatin1("base14-fonts.pdf"));
    _docs[QString::fromLatin1("metadata")] = backend.newDocument(QString::fromLatin1("metadata.pdf"));
    _docs[QString::fromLatin1("page-rotation")] = backend.newDocument(QString::fromLatin1("page-rotation.pdf"));
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
  newDocTest("page-rotation") << true;
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
  newDocTest("page-rotation") << false;
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
  newDocTest("page-rotation") << 4;
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
  newDocTest("transitions") << QString::fromLatin1("pdf-transitions.pdf");
  newDocTest("pgfmanual") << QString::fromLatin1("pgfmanual.pdf");
  newDocTest("base14-fonts") << QString::fromLatin1("base14-fonts.pdf");
  newDocTest("base14-locked") << QString::fromLatin1("base14-fonts-locked.pdf");
  newDocTest("metadata") << QString::fromLatin1("metadata.pdf");
  newDocTest("page-rotation") << QString::fromLatin1("page-rotation.pdf");
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
  QTest::addColumn<QVariant>("pageSize");

  newDocTest("invalid") << QVariant(QSizeF());
  newDocTest("transitions") << QVariant(QSizeF(362.835, 272.126));
  newDocTest("pgfmanual") << QVariant(QSizeF(595.276, 841.89));
  newDocTest("base14-fonts") << QVariant(QSizeF(595, 842));
  newDocTest("base14-locked") << QVariant(QSizeF(595, 842));
  newDocTest("metadata") << QVariant(QSizeF(612, 792));
  newDocTest("page-rotation") << QVariant::fromValue(QVariantList() << QVariant(QSizeF(595, 842)) << QVariant(QSizeF(842, 595)) << QVariant(QSizeF(595, 842)) << QVariant(QSizeF(842, 595)));
}

void TestQtPDF::page()
{
  QFETCH(pDoc, doc);
  QFETCH(QVariant, pageSize);
  
  QList<QSizeF> pageSizes;
  int i;

  QVERIFY(!doc.isNull());
  QVERIFY(doc->page(-1).isNull());
  QVERIFY(doc->page(doc->numPages()).isNull());

  if (pageSize.type() == QVariant::SizeF) {
    for (i = 0; i < doc->numPages(); ++i)
      pageSizes.append(pageSize.toSizeF());
  }
  else if (pageSize.type() == QVariant::List) {
    QVariantList l(pageSize.value<QVariantList>());
    while (pageSizes.length() < doc->numPages()) {
      for (i = 0; i < l.length(); ++i)
        pageSizes.append(l[i].value<QSizeF>());
    }
  }
  else {
    QFAIL(pageSize.typeName());
  }
  
  for (i = 0; i < doc->numPages(); ++i) {
    QSharedPointer<QtPDF::Backend::Page> page = doc->page(i).toStrongRef();
    QSizeF size = pageSizes[i];

    QVERIFY(!page.isNull());
    QVERIFY(page->pageNum() == i);
#ifdef USE_POPPLERQT
    QEXPECT_FAIL("base14-locked", "poppler-qt doesn't report page sizes for locked documents", Continue);
#endif
    QVERIFY2(qAbs(page->pageSizeF().width() - size.width()) < 1e-4, qPrintable(QString::fromLatin1("Width of page %1 is %2 instead of %3").arg(i + 1).arg(page->pageSizeF().width()).arg(size.width())));
    QVERIFY2(qAbs(page->pageSizeF().height() - size.height()) < 1e-4, qPrintable(QString::fromLatin1("Height of page %1 is %2 instead of %3").arg(i + 1).arg(page->pageSizeF().height()).arg(size.height())));

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
  newDocTest("metadata") << QString::fromLatin1("PDF Test File");
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
  newDocTest("metadata") << QString::fromLatin1("pdf, metadata, test");
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
  newDocTest("metadata") << QString::fromLatin1("gedit");
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
  newDocTest("metadata") << QString::fromLatin1("also gedit");
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
