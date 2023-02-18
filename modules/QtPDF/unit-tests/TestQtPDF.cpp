/*
  This is part of TeXworks, an environment for working with TeX documents
  Copyright (C) 2013-2023  Stefan Löffler

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
#include "TestQtPDF.h"
#include "PaperSizes.h"
#include "PhysicalUnits.h"

#ifdef USE_MUPDF
  typedef QtPDF::MuPDFBackend Backend;
#elif USE_POPPLERQT
  typedef QtPDF::PopplerQtBackend Backend;
#else
  #error Must specify one backend
#endif

namespace QTest {

#ifdef DEBUG

// Starting with QT_VERSION_CHECK(5, 5, 0), this could (should) be changed from
// a template instanciation in the QTest namespace to a function overload in the
// type's namespace - see docs of QTest::toString()
template<>
char * toString(const QtPDF::PDFDestination & dst) {
  QString buffer;
  QDebug dbg(&buffer);
  dbg << dst;
  return QTest::toString(buffer);
}
template<>
char * toString(const QtPDF::PDFAction & act) {
  QString buffer;
  QDebug dbg(&buffer);
  dbg << act;
  return QTest::toString(buffer);
}

#endif // defined(DEBUG)

} // namespace QTest

namespace UnitTest {

class ComparableImage : public QImage {
  double _threshold;
public:
  explicit ComparableImage(const QImage & other, const double threshold = 3) : QImage(other.convertToFormat(QImage::Format_RGB32)), _threshold(threshold) { }
  explicit ComparableImage(const QString & filename, const double threshold = 3) : QImage(QImage(filename).convertToFormat(QImage::Format_RGB32)), _threshold(threshold) { }

  bool isHomogeneous() const {
    if (size().isEmpty()) {
      return true;
    }
    Q_ASSERT(format() == QImage::Format_RGB32);

    QRgb refColor = pixel(0, 0);
    for (int j = 0; j < height(); ++j) {
      const uchar * row = constScanLine(j);
      for (int i = 0; i < 3 * width(); i += 3) {
        uchar r = row[i];
        uchar g = row[i + 1];
        uchar b = row[i + 2];
        if (qRgb(r, g, b) != refColor) {
          return false;
        }
      }
    }
    return true;
  }

  bool operator==(const ComparableImage & other) const {
    Q_ASSERT(format() == QImage::Format_RGB32);
    Q_ASSERT(other.format() == QImage::Format_RGB32);

    double threshold = qMax(_threshold, other._threshold);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    if (byteCount() != other.byteCount()) return false;
#else
    if (sizeInBytes() != other.sizeInBytes()) return false;
#endif

    double diff = 0.0;
    const uchar * src = bits();
    const uchar * dst = other.bits();
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    for (int i = 0; i < byteCount(); ++i)
      diff += qAbs(static_cast<int>(src[i]) - static_cast<int>(dst[i]));
#else
    for (qsizetype i = 0; i < sizeInBytes(); ++i)
      diff += qAbs(static_cast<int>(src[i]) - static_cast<int>(dst[i]));
#endif

    diff /= size().width() * size().height();

    if (diff >= threshold)
      qDebug() << "Difference" << diff << ">=" << threshold;

    return diff < threshold;
  }
};

class GenericDocument;

class GenericPage : public QtPDF::Backend::Page
{
public:
  GenericPage(GenericDocument * parent, int at, QSharedPointer<QReadWriteLock> docLock);
  QSizeF pageSizeF() const override { return {}; }
  QList<QSharedPointer<QtPDF::Annotation::Link> > loadLinks() override { return {}; }
  QList<QtPDF::Backend::SearchResult> search(const QString &searchText, const QtPDF::Backend::SearchFlags &flags) const override {
    Q_UNUSED(searchText) Q_UNUSED(flags) return {};
  }
  QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false) const override {
    Q_UNUSED(xres) Q_UNUSED(yres) Q_UNUSED(render_box) Q_UNUSED(cache)
    return {};
  }
};

class GenericDocument : public QtPDF::Backend::Document
{
public:
  GenericDocument(const QString & filename = QString()) : QtPDF::Backend::Document(filename) {
    _numPages = 1;
    _pages.append(QSharedPointer<QtPDF::Backend::Page>(new GenericPage(this, 0, _docLock)));
  }
  bool isValid() const override { return true; }
  bool isLocked() const override { return false; }
  bool unlock(const QString password) override { Q_UNUSED(password) return true; }
  void reload() override { }
};

GenericPage::GenericPage(GenericDocument * parent, int at, QSharedPointer<QReadWriteLock> docLock) : QtPDF::Backend::Page(parent, at, docLock) { }

inline void sleep(int ms)
{
#ifdef Q_OS_MACOS
  // QTest::qSleep seems very unreliable on Mac OS X (QTBUG-84998)
  QElapsedTimer t;
  t.start();
  qint64 dt = (ms < 100 ? 1 : ms / 100);
  while(t.elapsed() < ms) {
    QTest::qSleep(dt);
  }
  // Issue a warning if the timing is off by more than 20%
  if (t.elapsed() > 1.2 * ms) {
    qWarning() << t.elapsed() << "ms have passed instead of the requested" << ms << "ms";
  }
#else
  QTest::qSleep(ms);
#endif
}

QTestData & TestQtPDF::newDocTest(const char * tag)
{
  return QTest::newRow(tag) << _docs[QString::fromUtf8(tag)];
}

QTestData & TestQtPDF::newPageTest(const char * tag, const int iPage)
{
  return QTest::newRow(qPrintable(QString::fromUtf8(tag) + QString::fromLatin1(" p") + QString::number(iPage + 1))) << _docs[QString::fromUtf8(tag)]->page(iPage).toStrongRef();
}

void TestQtPDF::backendInterface()
{
#ifdef USE_MUPDF
  QtPDF::MuPDFBackend backend;
  QCOMPARE(backend.name(), QString::fromLatin1("mupdf"));
#elif USE_POPPLERQT
  QtPDF::PopplerQtBackend backend;
  QCOMPARE(backend.name(), QString::fromLatin1("poppler-qt"));
#else
  #error Must specify one backend
#endif

  QVERIFY(backend.canHandleFile(QString::fromLatin1("test.pdf")));
  QVERIFY(!backend.canHandleFile(QString::fromLatin1("test.tex")));
}

void TestQtPDF::abstractBaseClasses()
{
  QString filename = QStringLiteral("test.pdf");
  GenericDocument doc(filename);
  QMap<QString, QString> metaDataOther;
  QtPDF::PDFDestination dstExplicit(0), dstNamed(QStringLiteral("name"));

  // Document
  QCOMPARE(doc.numPages(), 1);
  QCOMPARE(doc.fileName(), filename);
  QCOMPARE(doc.resolveDestination(dstExplicit), dstExplicit);
  QCOMPARE(doc.resolveDestination(dstNamed), QtPDF::PDFDestination());
  QCOMPARE(doc.permissions(), QtPDF::Backend::Document::Permissions());
  QCOMPARE(doc.isValid(), true);
  QCOMPARE(doc.isLocked(), false);
  doc.reload();
  QCOMPARE(doc.unlock(QStringLiteral()), true);
  QCOMPARE(doc.toc(), QtPDF::Backend::PDFToC());
  QCOMPARE(doc.fonts(), QList<QtPDF::Backend::PDFFontInfo>());
  QCOMPARE(doc.title(), QString());
  QCOMPARE(doc.author(), QString());
  QCOMPARE(doc.subject(), QString());
  QCOMPARE(doc.keywords(), QString());
  QCOMPARE(doc.creator(), QString());
  QCOMPARE(doc.producer(), QString());
  QCOMPARE(doc.creationDate(), QDateTime());
  QCOMPARE(doc.modDate(), QDateTime());
  QCOMPARE(doc.pageSize(), QSizeF());
  QCOMPARE(doc.fileSize(), static_cast<qint64>(0));
  QCOMPARE(doc.trapped(), QtPDF::Backend::Document::Trapped_Unknown);
  QCOMPARE(doc.metaDataOther(), metaDataOther);
  QCOMPARE(doc.search(QStringLiteral(), {}), QList<QtPDF::Backend::SearchResult>());

  // Page
  QSharedPointer<QtPDF::Backend::Page> page = doc.page(0).toStrongRef();
  QVERIFY(!page.isNull());
  QCOMPARE(page->document(), &doc);
  QCOMPARE(page->pageNum(), 0);
  QCOMPARE(page->pageSizeF(), QSizeF());
  QCOMPARE(page->getContentBoundingBox(), QRectF());
  QVERIFY(page->transition() == nullptr);
  QCOMPARE(page->loadLinks(), QList< QSharedPointer<QtPDF::Annotation::Link> >());
  QCOMPARE(page->boxes(), QList<QtPDF::Backend::Page::Box>());

  QMap<int, QRectF> wordBoxes,  charBoxes;
  wordBoxes.insert(0, QRectF());
  charBoxes.insert(0, QRectF());
  QCOMPARE(page->selectedText({}, &wordBoxes, &charBoxes), QString());
  QVERIFY(wordBoxes.isEmpty());
  QVERIFY(charBoxes.isEmpty());

  QVERIFY(page->renderToImage(1, 1).isNull());
  QVERIFY(page->getTileImage(nullptr, 1, 1).isNull());

  QCOMPARE(page->loadAnnotations(), QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> >());
  QCOMPARE(page->search(QStringLiteral(), {}), QList<QtPDF::Backend::SearchResult>());
}

void TestQtPDF::loadDocs()
{
  Backend backend;

  // Don't run documents that may produce error messages in QBENCHMARK as
  // otherwise the console may be filled with unhelpful messages
  _docs[QString::fromLatin1("invalid")] = backend.newDocument(QString());
  _docs[QString::fromLatin1("base14-locked")] = backend.newDocument(QString::fromLatin1("base14-fonts-locked.pdf"));

  QBENCHMARK {
    _docs[QString::fromLatin1("transitions")] = backend.newDocument(QString::fromLatin1("pdf-transitions.pdf"));
    _docs[QString::fromLatin1("pgfmanual")] = backend.newDocument(QString::fromLatin1("pgfmanual.pdf"));
    _docs[QString::fromLatin1("base14-fonts")] = backend.newDocument(QString::fromLatin1("base14-fonts.pdf"));
    _docs[QString::fromLatin1("poppler-data")] = backend.newDocument(QString::fromLatin1("poppler-data.pdf"));
    _docs[QString::fromLatin1("metadata")] = backend.newDocument(QString::fromLatin1("metadata.pdf"));
    _docs[QString::fromLatin1("page-rotation")] = backend.newDocument(QString::fromLatin1("page-rotation.pdf"));
    _docs[QString::fromLatin1("annotations")] = backend.newDocument(QString::fromLatin1("annotations.pdf"));
    _docs[QString::fromLatin1("jpg")] = backend.newDocument(QStringLiteral("jpg.pdf"));
  }
}

void TestQtPDF::parsePDFDate_data()
{
  QTest::addColumn<QString>("str");
  QTest::addColumn<QDateTime>("result");

  // NB: fromPDFDate always returns local time
  QTest::newRow("empty") << QString() << QDateTime();
  QTest::newRow("yyyy") << QStringLiteral("D:2000") << QDateTime(QDate(2000, 1, 1), QTime());
  QTest::newRow("yyyymm") << QStringLiteral("D:200002") << QDateTime(QDate(2000, 2, 1), QTime());
  QTest::newRow("yyyymmdd") << QStringLiteral("D:20000202") << QDateTime(QDate(2000, 2, 2), QTime());
  QTest::newRow("yyyymmddHH") << QStringLiteral("D:2000020213") << QDateTime(QDate(2000, 2, 2), QTime(13, 0, 0));
  QTest::newRow("yyyymmddHHMM") << QStringLiteral("D:200002021342") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 0));
  QTest::newRow("yyyymmddHHMMSS") << QStringLiteral("D:20000202134221") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 21));
  QTest::newRow("yyyymmddHHMMSSZ") << QStringLiteral("D:20000202134221Z") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 21), Qt::UTC).toLocalTime();
  QTest::newRow("yyyymmddHHMMSS+07'30") << QStringLiteral("D:20000202134221+07'30") << QDateTime(QDate(2000, 2, 2), QTime(6, 12, 21), Qt::UTC).toLocalTime();
  QTest::newRow("yyyymmddHHMMSS-08'00") << QStringLiteral("D:20000202134221-08'00") << QDateTime(QDate(2000, 2, 2), QTime(21, 42, 21), Qt::UTC).toLocalTime();
  QTest::newRow("yyyymmddHHMMSS;08'00") << QStringLiteral("D:20000202134221;08'00") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 21));
  QTest::newRow("yyyymmddHHMMSS-0800") << QStringLiteral("D:20000202134221-0800") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 21));
  QTest::newRow("yyyymmddHHMMSS-0a'00") << QStringLiteral("D:20000202134221-0a'00") << QDateTime(QDate(2000, 2, 2), QTime(13, 42, 21));
}

void TestQtPDF::parsePDFDate()
{
  QFETCH(QString, str);
  QFETCH(QDateTime, result);

  QCOMPARE(QtPDF::Backend::fromPDFDate(str), result);
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

void TestQtPDF::unlock_data()
{
  // For testing unlock(), we must freshly load the pdf files each time to
  // ensure that a previously successful unlock does not interfer with a later
  // unlock attempt. Also, unlocking the cached "base14-locked" document would
  // cause subsequent test cases (which assume it is locked) to fail.
  QTest::addColumn<QString>("filename");
  QTest::addColumn<QString>("pwd");
  QTest::addColumn<bool>("success");

  QTest::newRow("unlocked") << QString::fromLatin1("base14-fonts.pdf") << QString() << true;
  QTest::newRow("wrong-pwd") << QString::fromLatin1("base14-fonts-locked.pdf") << QString() << false;
  QTest::newRow("user-pwd") << QString::fromLatin1("base14-fonts-locked.pdf") << QString::fromLatin1("123") << true;
  QTest::newRow("owner-pwd") << QString::fromLatin1("base14-fonts-locked.pdf") << QString::fromLatin1("test") << true;
}

void TestQtPDF::unlock()
{
  QFETCH(QString, filename);
  QFETCH(QString, pwd);
  QFETCH(bool, success);
  Backend backend;

  pDoc doc = backend.newDocument(filename);
  QCOMPARE(doc->unlock(pwd), success);
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

  QVERIFY(!doc.isNull());
  QVERIFY(doc->page(-1).isNull());
  QVERIFY(doc->page(doc->numPages()).isNull());

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  const bool isSizeF = (pageSize.type() == QVariant::SizeF);
  const bool isVariantList = (pageSize.type() == QVariant::List);
#else
  const bool isSizeF = (pageSize.metaType().id() == QMetaType::QSizeF);
  const bool isVariantList = (pageSize.metaType().id() == QMetaType::QVariantList);
#endif

  if (isSizeF) {
    for (int i = 0; i < doc->numPages(); ++i)
      pageSizes.append(pageSize.toSizeF());
  }
  else if (isVariantList) {
    QVariantList l(pageSize.value<QVariantList>());
    while (pageSizes.length() < doc->numPages()) {
      for (int i = 0; i < l.length(); ++i)
        pageSizes.append(l[i].toSizeF());
    }
  }
  else {
    QFAIL(pageSize.typeName());
  }

  for (int i = 0; i < doc->numPages(); ++i) {
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

void TestQtPDF::destination_data()
{
  QTest::addColumn<QtPDF::PDFDestination>("dst");
  QTest::addColumn<bool>("isValid");
  QTest::addColumn<bool>("isExplicit");
  QTest::addColumn<int>("page");
  QTest::addColumn<QString>("name");
  QTest::addColumn<QtPDF::PDFDestination::Type>("type");
  QTest::addColumn<qreal>("zoom");
  QTest::addColumn<QRectF>("rect");
  QTest::addColumn<QRectF>("nullptrRect");
  QTest::addColumn<QRectF>("invalidRect");
  QTest::addColumn<QRectF>("base14Rect");
  QTest::addColumn<QRectF>("zoom1Rect");
  QTest::addColumn<QRectF>("zoom2Rect");
  QTest::addColumn<QString>("dbgOutput");

  QtPDF::PDFDestination dst;
  QRectF viewport{1, 2, 3, 4};
  QString dstName = QStringLiteral("page.2");
  QRectF invalidRect{QPointF(-1, -1), QSizeF(-1, -1)};

  // Check defaults
  QTest::newRow("default") << dst << false << false << -1 << QStringLiteral() <<
    QtPDF::PDFDestination::Type::Destination_XYZ << -1. <<
    invalidRect << viewport << viewport << viewport << viewport << viewport <<
    QStringLiteral("PDFDestination()");

  // Check a named destination
  dst.setDestinationName(dstName);
  QTest::newRow("named") << dst << true << false << -1 << dstName <<
    QtPDF::PDFDestination::Type::Destination_XYZ << -1. <<
    invalidRect << viewport << viewport << viewport << QRectF(QPointF(102, 750.89), QSizeF(3, 4)) << QRectF(QPointF(102, 750.89), QSizeF(3, 4)) <<
    QStringLiteral("PDFDestination(name=\"%1\")").arg(dstName);

  // Unsetting the name should yield an invalid destination again
  dst.setDestinationName(QStringLiteral());
  QTest::newRow("reset") << dst << false << false << -1 << QStringLiteral() <<
    QtPDF::PDFDestination::Type::Destination_XYZ << -1. <<
    invalidRect << viewport << viewport << viewport << viewport << viewport <<
    QStringLiteral("PDFDestination()");

  // Check explicit destinations
  dst.setPage(0);
  dst.setRect(viewport);
  dst.setZoom(1.5);
  {
    QRectF zoom1Rect{QPointF(1.5, 3), QSizeF(4.5, 6)};
    QRectF zoom2Rect{QPointF(0.75, 1.5), QSizeF(2.25, 3)};

    // XYZ destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_XYZ);
    QTest::newRow("XYZ") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_XYZ << 1.5 <<
      viewport << zoom1Rect << zoom1Rect << zoom1Rect << zoom1Rect << zoom2Rect <<
      QStringLiteral("PDFDestination(%1 /XYZ %2 %3 %4)").arg(dst.page()).arg(dst.left()).arg(dst.top()).arg(dst.zoom());
  }

  {
    QRectF A4Rect{QPointF(0, 0), QSizeF(595.276, 841.89)};
    QRectF A4RectRounded{QPointF(0, 0), QSizeF(595, 842)};

    // Fit destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_Fit);
    QTest::newRow("Fit") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_Fit << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /Fit)").arg(dst.page());

    // FitB destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitB);
    QTest::newRow("FitB") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitB << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /FitB)").arg(dst.page());
  }

  {
    QRectF A4Rect{QPointF(0, 0), QSizeF(595.276, 793.701333333)};
    QRectF A4RectRounded{QPointF(0, 0), QSizeF(595, 793.333333333)};

    // FitH destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitH);
    QTest::newRow("FitH") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitH << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /FitH %2)").arg(dst.page()).arg(dst.top());

    // FitBH destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitBH);
    QTest::newRow("FitBH") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitBH << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /FitBH %2)").arg(dst.page()).arg(dst.top());
  }

  {
    QRectF A4Rect{QPointF(0, 0), QSizeF(595.276, 841.89)};
    QRectF A4RectRounded{QPointF(0, 0), QSizeF(595, 842)};

    // FitV destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitV);
    QTest::newRow("FitV") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitV << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /FitV %2)").arg(dst.page()).arg(dst.left());

    // FitBV destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitBV);
    QTest::newRow("FitBV") << dst << true << true << 0 << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitBV << 1.5 <<
      viewport << viewport << viewport << A4RectRounded << A4Rect << A4Rect <<
      QStringLiteral("PDFDestination(%1 /FitBV %2)").arg(dst.page()).arg(dst.left());
  }

  // FitR destination
  dst.setType(QtPDF::PDFDestination::Type::Destination_FitR);
  QTest::newRow("FitR") << dst << true << true << 0 << QStringLiteral() <<
    QtPDF::PDFDestination::Type::Destination_FitR << 1.5 <<
    viewport << viewport << viewport << viewport << viewport << viewport <<
    QStringLiteral("PDFDestination(%1 /FitR %2 %3 %4 %5)").arg(dst.page()).arg(dst.left()).arg(dst.rect().bottom()).arg(dst.rect().right()).arg(dst.top());

  // Check explicit destinations with a page number that doesn't exist
  constexpr int invalidPageNum = 10000;
  dst.setPage(invalidPageNum);
  {
    QRectF zoom1Rect{QPointF(1.5, 3), QSizeF(4.5, 6)};
    QRectF zoom2Rect{QPointF(0.75, 1.5), QSizeF(2.25, 3)};

    // XYZ destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_XYZ);
    QTest::newRow("XYZ invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_XYZ << 1.5 <<
      viewport << zoom1Rect << zoom1Rect << zoom1Rect << zoom1Rect << zoom2Rect <<
      QStringLiteral("PDFDestination(%1 /XYZ %2 %3 %4)").arg(dst.page()).arg(dst.left()).arg(dst.top()).arg(dst.zoom());
  }

  {
    // Fit destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_Fit);
    QTest::newRow("Fit invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_Fit << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /Fit)").arg(dst.page());

    // FitB destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitB);
    QTest::newRow("FitB invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitB << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /FitB)").arg(dst.page());
  }

  {
    // FitH destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitH);
    QTest::newRow("FitH invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitH << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /FitH %2)").arg(dst.page()).arg(dst.top());

    // FitBH destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitBH);
    QTest::newRow("FitBH invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitBH << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /FitBH %2)").arg(dst.page()).arg(dst.top());
  }

  {
    // FitV destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitV);
    QTest::newRow("FitV invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitV << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /FitV %2)").arg(dst.page()).arg(dst.left());

    // FitBV destination
    dst.setType(QtPDF::PDFDestination::Type::Destination_FitBV);
    QTest::newRow("FitBV invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
      QtPDF::PDFDestination::Type::Destination_FitBV << 1.5 <<
      viewport << viewport << viewport << viewport << viewport << viewport <<
      QStringLiteral("PDFDestination(%1 /FitBV %2)").arg(dst.page()).arg(dst.left());
  }

  // FitR destination
  dst.setType(QtPDF::PDFDestination::Type::Destination_FitR);
  QTest::newRow("FitR invalid page") << dst << true << true << invalidPageNum << QStringLiteral() <<
    QtPDF::PDFDestination::Type::Destination_FitR << 1.5 <<
    viewport << viewport << viewport << viewport << viewport << viewport <<
    QStringLiteral("PDFDestination(%1 /FitR %2 %3 %4 %5)").arg(dst.page()).arg(dst.left()).arg(dst.rect().bottom()).arg(dst.rect().right()).arg(dst.top());
}

void TestQtPDF::destination()
{
  QFETCH(QtPDF::PDFDestination, dst);
  QFETCH(bool, isValid);
  QFETCH(bool, isExplicit);
  QFETCH(int, page);
  QFETCH(QString, name);
  QFETCH(QtPDF::PDFDestination::Type, type);
  QFETCH(qreal, zoom);
  QFETCH(QRectF, rect);
  QFETCH(QRectF, nullptrRect);
  QFETCH(QRectF, invalidRect);
  QFETCH(QRectF, base14Rect);
  QFETCH(QRectF, zoom1Rect);
  QFETCH(QRectF, zoom2Rect);
  QFETCH(QString, dbgOutput);

  QCOMPARE(dst.isValid(), isValid);
  QCOMPARE(dst.isExplicit(), isExplicit);
  QCOMPARE(dst.page(), page);
  QCOMPARE(dst.destinationName(), name);
  QCOMPARE(dst.type(), type);
  QCOMPARE(dst.zoom(), zoom);
  QCOMPARE(dst.rect(), rect);
  QCOMPARE(dst.top(), rect.top());
  QCOMPARE(dst.left(), rect.left());

  QRectF viewport{QPointF{1, 2}, QSizeF{3, 4}};
  QCOMPARE(dst.viewport(nullptr, viewport, 1), nullptrRect);
  QCOMPARE(dst.viewport(_docs[QStringLiteral("invalid")].data(), viewport, 1), invalidRect);
  QCOMPARE(dst.viewport(_docs[QStringLiteral("base14-fonts")].data(), viewport, 1), base14Rect);
  QCOMPARE(dst.viewport(_docs[QStringLiteral("annotations")].data(), viewport, 1), zoom1Rect);
  QCOMPARE(dst.viewport(_docs[QStringLiteral("annotations")].data(), viewport, 2), zoom2Rect);
#ifdef DEBUG
  QString buffer;
  QDebug dbg(&buffer);
  dbg << dst;
  QCOMPARE(buffer, dbgOutput);
#endif
}

void TestQtPDF::destinationComparison()
{
  QVector<QtPDF::PDFDestination> dests;

  dests << QtPDF::PDFDestination() << QtPDF::PDFDestination(QStringLiteral("name")) << QtPDF::PDFDestination(0);

  for (int i = 0; i < dests.length(); ++i) {
    QVERIFY(dests[i] == dests[i]);
    for (int j = i + 1; j < dests.length(); ++j) {
      QVERIFY2(!(dests[i] == dests[j]), qPrintable(QStringLiteral("dests[%1] == dests[%2]").arg(i).arg(j)));
    }
  }
}

void TestQtPDF::PDFUriAction()
{
  QUrl urlTw(QStringLiteral("http://www.tug.org/texworks/"));
  QtPDF::PDFURIAction a(urlTw);

  QCOMPARE(a.type(), QtPDF::PDFAction::ActionTypeURI);
  QCOMPARE(a.url(), urlTw);

  QtPDF::PDFAction * c = a.clone();
  Q_ASSERT(c != nullptr);
  QVERIFY(c != &a);
  QCOMPARE(*c, dynamic_cast<QtPDF::PDFAction&>(a));
  delete c;
}

void TestQtPDF::PDFGotoAction()
{
  QtPDF::PDFDestination dst(0);
  QString filename(QStringLiteral("test.pdf"));
  QtPDF::PDFGotoAction a;

  // Defaults
  QCOMPARE(a.type(), QtPDF::PDFAction::ActionTypeGoTo);
  QCOMPARE(a.destination(), QtPDF::PDFDestination());
  QCOMPARE(a.isRemote(), false);
  QCOMPARE(a.filename(), QString());
  QCOMPARE(a.openInNewWindow(), false);

  a.setDestination(dst);
  QCOMPARE(a.destination(), dst);

  a.setRemote(true);
  QCOMPARE(a.isRemote(), true);

  a.setFilename(filename);
  QCOMPARE(a.filename(), filename);

  a.setOpenInNewWindow(true);
  QCOMPARE(a.openInNewWindow(), true);

  QtPDF::PDFAction * c = a.clone();
  Q_ASSERT(c != nullptr);
  QVERIFY(c != &a);
  QCOMPARE(*c, dynamic_cast<QtPDF::PDFAction&>(a));
  delete c;
}

void TestQtPDF::PDFLaunchAction()
{
  QString empty, cmd(QStringLiteral("cmd"));
  QtPDF::PDFLaunchAction a(empty);

  QCOMPARE(a.type(), QtPDF::PDFAction::ActionTypeLaunch);
  QCOMPARE(a.command(), empty);
  a.setCommand(cmd);
  QCOMPARE(a.command(), cmd);

  QtPDF::PDFAction * c = a.clone();
  Q_ASSERT(c != nullptr);
  QVERIFY(c != &a);
  QCOMPARE(*c, dynamic_cast<QtPDF::PDFAction&>(a));
  delete c;
}

void TestQtPDF::actionComparison()
{
  // Can't use QScopedPointer here as that does not work with QVector's <<
  using QSP = QSharedPointer<QtPDF::PDFAction>;
  QVector< QSP > actions;

  actions << QSP(new QtPDF::PDFURIAction(QUrl()))
          << QSP(new QtPDF::PDFURIAction(QUrl(QStringLiteral("http://www.tug.org/texworks/"))))
          << QSP(new QtPDF::PDFGotoAction())
          << QSP(new QtPDF::PDFGotoAction(QtPDF::PDFDestination(0)))
          << QSP(new QtPDF::PDFGotoAction(QtPDF::PDFDestination(QStringLiteral("name"))))
          << QSP(new QtPDF::PDFLaunchAction(QStringLiteral()))
          << QSP(new QtPDF::PDFLaunchAction(QStringLiteral("cmd")));

  for (int i = 0; i < actions.size(); ++i) {
    for (int j = 0; j < actions.size(); ++j) {
      if (i == j) {
        QCOMPARE(*actions[i], *actions[i]);
      }
      else {
        QVERIFY2(!(*actions[i] == *actions[j]), qPrintable(QStringLiteral("actions[%1] == actions[%2]").arg(i).arg(j)));
      }
    }
  }
}

void TestQtPDF::resolveDestination_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QtPDF::PDFDestination>("src");
  QTest::addColumn<QtPDF::PDFDestination>("dest");

  {
    QtPDF::PDFDestination d;
    d.setDestinationName(QString::fromLatin1("does not exist"));
    newDocTest("annotations") << d << QtPDF::PDFDestination();
  }
  {
    QtPDF::PDFDestination d(1);
    QtPDF::PDFDestination n(1);
    d.setRect(QRectF(102, 750.89, -1, -1));
    n.setDestinationName(QString::fromLatin1("page.2"));
    newDocTest("annotations") << d << d;
    newDocTest("annotations") << n << d;
  }
}

void TestQtPDF::resolveDestination()
{
  QFETCH(pDoc, doc);
  QFETCH(QtPDF::PDFDestination, src);
  QFETCH(QtPDF::PDFDestination, dest);

  QtPDF::PDFDestination actual = doc->resolveDestination(src);

  QCOMPARE(actual, dest);
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
  newDocTest("pgfmanual") << QDateTime(QDate(2010, 10, 25), QTime(20, 56, 26), Qt::UTC);
  newDocTest("base14-fonts") << QDateTime();
  newDocTest("base14-locked") << QDateTime();
  newDocTest("metadata") << QDateTime(QDate(2013, 9, 7), QTime(23, 23, 45), Qt::UTC);
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
  newDocTest("pgfmanual") << QDateTime(QDate(2010, 10, 25), QTime(20, 56, 26), Qt::UTC);
  newDocTest("base14-fonts") << QDateTime();
  newDocTest("base14-locked") << QDateTime();
  newDocTest("metadata") << QDateTime(QDate(2013, 9, 8), QTime(10, 34, 56), Qt::UTC);
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
    newDocTest("invalid") << static_cast<int>(QtPDF::Backend::Document::Trapped_Unknown);
    newDocTest("transitions") << static_cast<int>(QtPDF::Backend::Document::Trapped_Unknown);
    newDocTest("pgfmanual") << static_cast<int>(QtPDF::Backend::Document::Trapped_False);
    newDocTest("base14-fonts") << static_cast<int>(QtPDF::Backend::Document::Trapped_Unknown);
    newDocTest("base14-locked") << static_cast<int>(QtPDF::Backend::Document::Trapped_Unknown);
    newDocTest("metadata") << static_cast<int>(QtPDF::Backend::Document::Trapped_Unknown);
}

void TestQtPDF::metaDataTrapped()
{
    QFETCH(pDoc, doc);
    QFETCH(int, expected);
#ifdef USE_POPPLERQT
    QEXPECT_FAIL("pgfmanual", "poppler-qt doesn't handle trapping properly", Continue);
#endif
    QCOMPARE(static_cast<int>(doc->trapped()), expected);
}

void TestQtPDF::metaDataOther_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QStringMap>("data");

  newDocTest("invalid") << QStringMap();

  {
    QStringMap a;
    a.insert(QString::fromLatin1("PTEX.FullBanner"), QString::fromLatin1("This is LuaTeX, Version 1.07.0 (TeX Live 2018)"));
    newDocTest("annotations") << a;
  }
}

void TestQtPDF::metaDataOther()
{
  QFETCH(pDoc, doc);
  QFETCH(QStringMap, data);

  QCOMPARE(doc->metaDataOther(), data);
}

void TestQtPDF::fileSize_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<qint64>("filesize");

  newDocTest("invalid") << static_cast<qint64>(0);
  newDocTest("base14-fonts") << static_cast<qint64>(3800);
  newDocTest("base14-locked") << static_cast<qint64>(2774);
  newDocTest("pgfmanual") << static_cast<qint64>(5346838);
  newDocTest("annotations") << static_cast<qint64>(11817);
}

void TestQtPDF::fileSize()
{
  QFETCH(pDoc, doc);
  QFETCH(qint64, filesize);

  QCOMPARE(doc->fileSize(), filesize);
}

void TestQtPDF::pageSize_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QSizeF>("size");

  newDocTest("invalid") << QSizeF();
  newDocTest("base14-fonts") << QSizeF(595, 842);
  newDocTest("base14-locked") << QSizeF();
  newDocTest("pgfmanual") << QSizeF(595.276, 841.89);
  newDocTest("page-rotation") << QSizeF(595, 842);
  newDocTest("transitions") << QSizeF(362.835, 272.126);
}

void TestQtPDF::pageSize()
{
  QFETCH(pDoc, doc);
  QFETCH(QSizeF, size);

  QCOMPARE(doc->pageSize(), size);
}

void TestQtPDF::permissions_data()
{
  using namespace QtPDF::Backend;

  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<Document::Permissions>("permissions");

  newDocTest("invalid") << Document::Permissions();
  newDocTest("base14-locked") << Document::Permissions();
  newDocTest("base14-fonts") << Document::Permissions(Document::Permission_Annotate |
                                                      Document::Permission_Assemble |
                                                      Document::Permission_Change |
                                                      Document::Permission_Extract |
                                                      Document::Permission_ExtractForAccessibility |
                                                      Document::Permission_FillForm |
                                                      Document::Permission_Print |
                                                      Document::Permission_PrintHighRes);
}

void TestQtPDF::permissions()
{
  QFETCH(pDoc, doc);
  QFETCH(QtPDF::Backend::Document::Permissions, permissions);

  QCOMPARE(doc->permissions(), permissions);
}

void TestQtPDF::fontDescriptor_data()
{
  QTest::addColumn<QString>("fontName");
  QTest::addColumn<QString>("pureName");
  QTest::addColumn<bool>("isSubset");

  QTest::newRow("default") << QString() << QString() << false;
  QTest::newRow("full") << QStringLiteral("Font") << QStringLiteral("Font") << false;
  QTest::newRow("subset") << QStringLiteral("ABCDEF+font") << QStringLiteral("font") << true;
  QTest::newRow("not-subset") << QStringLiteral("Font56+") << QStringLiteral("Font56+") << false;
}

void TestQtPDF::fontDescriptor()
{
  QFETCH(QString, fontName);
  QFETCH(QString, pureName);
  QFETCH(bool, isSubset);

  QtPDF::Backend::PDFFontDescriptor fd;
  fd.setName(fontName);
  QCOMPARE(fd.name(), fontName);
  QCOMPARE(fd.pureName(), pureName);
  QCOMPARE(fd.isSubset(), isSubset);
}

void TestQtPDF::fontDescriptorComparison()
{
  QVector<QtPDF::Backend::PDFFontDescriptor> fds;

  fds << QtPDF::Backend::PDFFontDescriptor()
      << QtPDF::Backend::PDFFontDescriptor(QStringLiteral("font1"))
      << QtPDF::Backend::PDFFontDescriptor(QStringLiteral("Font2"))
      << QtPDF::Backend::PDFFontDescriptor(QStringLiteral("Font56+"))
      << QtPDF::Backend::PDFFontDescriptor(QStringLiteral("ABCDEF+font1"))
      << QtPDF::Backend::PDFFontDescriptor(QStringLiteral("ABCDEF+Font2"));
  for (int i = 0; i < fds.size(); ++i) {
    for (int j = 0; j < fds.size(); ++j) {
      if (i == j) {
        QCOMPARE(fds[i], fds[i]);
      }
      else {
        QVERIFY2(!(fds[i] == fds[j]), qPrintable(QStringLiteral("fds[%1] == fds[%2]").arg(i).arg(j)));
      }
    }
  }
}

void TestQtPDF::fonts_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QStringList>("fontNames");

  newDocTest("invalid") << QStringList();
  newDocTest("base14-locked") << QStringList();
  newDocTest("base14-fonts") << (QStringList() << QString::fromLatin1("Times-Roman")
                                               << QString::fromLatin1("Times-Bold")
                                               << QString::fromLatin1("Times-Italic")
                                               << QString::fromLatin1("Times-BoldItalic")
                                               << QString::fromLatin1("Helvetica")
                                               << QString::fromLatin1("Helvetica-Bold")
                                               << QString::fromLatin1("Helvetica-Oblique")
                                               << QString::fromLatin1("Helvetica-BoldOblique")
                                               << QString::fromLatin1("Courier")
                                               << QString::fromLatin1("Courier-Bold")
                                               << QString::fromLatin1("Courier-Oblique")
                                               << QString::fromLatin1("Courier-BoldOblique")
                                               << QString::fromLatin1("Symbol")
                                               << QString::fromLatin1("ZapfDingbats"));
  newDocTest("poppler-data") << (QStringList() << QString::fromLatin1("MHei-Bold-Identity-H")
                                               << QString::fromLatin1("MKai-SemiBold-Identity-H")
                                               << QString::fromLatin1("MSung-Light-Identity-H")
                                               << QString::fromLatin1("Helvetica-Bold")
                                               << QString::fromLatin1("Times-Roman"));
}

void TestQtPDF::fonts()
{
  QFETCH(pDoc, doc);
  QFETCH(QStringList, fontNames);

  QList< QtPDF::Backend::PDFFontInfo > fonts = doc->fonts();
  QStringList actualFontNames;

  for (int i = 0; i < fonts.size(); ++i)
    actualFontNames.append(fonts[i].descriptor().pureName());

  QCOMPARE(actualFontNames, fontNames);
}

void TestQtPDF::ToCItem()
{
  QtPDF::Backend::PDFToCItem ti, def, act;
  QString label(QStringLiteral("label"));

  act.setAction(std::unique_ptr<QtPDF::PDFAction>(new QtPDF::PDFGotoAction(QtPDF::PDFDestination(0))));

  // Defaults
  QCOMPARE(ti.label(), QString());
  QCOMPARE(ti.isOpen(), false);
  QVERIFY(ti.action() == nullptr);
  QCOMPARE(ti.color(), QColor());
  QCOMPARE(ti.children(), QList<QtPDF::Backend::PDFToCItem>());
  // ensure the const variant of PDFToCItem::flags() is called
  QCOMPARE(static_cast<const QtPDF::Backend::PDFToCItem&>(ti).flags(), QtPDF::Backend::PDFToCItem::PDFToCItemFlags());
  QVERIFY(ti == ti);
  QVERIFY(ti == def);
  QVERIFY(!(ti == act));

  // Setters
  ti.setLabel(label);
  QCOMPARE(ti.label(), label);
  QVERIFY(!(ti == def));
  QVERIFY(!(ti == act));
  ti = def;
  QVERIFY(ti == def);

  ti.setOpen();
  QCOMPARE(ti.isOpen(), true);
  QVERIFY(!(ti == def));
  QVERIFY(!(ti == act));
  ti = def;
  QVERIFY(ti == def);

  ti.setColor(Qt::red);
  QCOMPARE(ti.color(), QColor(Qt::red));
  QVERIFY(!(ti == def));
  QVERIFY(!(ti == act));
  ti = def;
  QVERIFY(ti == def);

  ti.flags() |= QtPDF::Backend::PDFToCItem::Flag_Bold;
  QCOMPARE(ti.flags(), QtPDF::Backend::PDFToCItem::PDFToCItemFlags(QtPDF::Backend::PDFToCItem::Flag_Bold));
  QVERIFY(!(ti == def));
  QVERIFY(!(ti == act));
  ti = def;
  QVERIFY(ti == def);

  QtPDF::PDFGotoAction actGoto1 = QtPDF::PDFGotoAction(QtPDF::PDFDestination(1));
  ti.setAction(std::unique_ptr<QtPDF::PDFAction>(new QtPDF::PDFGotoAction(actGoto1)));
  QVERIFY(ti.action() != nullptr);
  QCOMPARE(*ti.action(), dynamic_cast<const QtPDF::PDFAction&>(actGoto1));
  QVERIFY(!(ti == def));
  QVERIFY(!(ti == act));

  // Self-assignment (ensure it does not crash)
  ti = ti;
}

// static
void TestQtPDF::compareToC(const QtPDF::Backend::PDFToC & actual, const QtPDF::Backend::PDFToC & expected)
{
  QCOMPARE(actual.size(), expected.size());
  if (QTest::currentTestFailed()) return;

  for (int i = 0; i < actual.size(); ++i) {
    QCOMPARE(actual[i].label(), expected[i].label());
    compareToC(actual[i].children(), expected[i].children());
  }
}

// static
void TestQtPDF::printToC(const QtPDF::Backend::PDFToC & toc, const QString & indent)
{
  for (int i = 0; i < toc.size(); ++i) {
    qDebug() << qPrintable(indent + toc[i].label());
    printToC(toc[i].children(), indent + QString::fromLatin1("  "));
  }
}

void TestQtPDF::toc_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<QtPDF::Backend::PDFToC>("toc");

  newDocTest("invalid") << QtPDF::Backend::PDFToC();

  {
    QtPDF::Backend::PDFToC a;
    a << QtPDF::Backend::PDFToCItem(QString::fromLatin1("A"));
    a << QtPDF::Backend::PDFToCItem(QString::fromLatin1("LaTeX"));
    a[0].children() << QtPDF::Backend::PDFToCItem(QString::fromUtf8("aä®€"));
    newDocTest("annotations") << a;
  }
}

void TestQtPDF::toc()
{
  QFETCH(pDoc, doc);
  QFETCH(QtPDF::Backend::PDFToC, toc);

  compareToC(doc->toc(), toc);

  if (QTest::currentTestFailed()) {
    qDebug() << "Actual:";
    printToC(doc->toc());
    qDebug() << "Expected:";
    printToC(toc);
  }
}

void TestQtPDF::annotationComparison()
{
  using SAP = QSharedPointer<QtPDF::Annotation::AbstractAnnotation>;
  QVector<SAP> annots;

  annots << SAP(new QtPDF::Annotation::Link())
         << SAP(new QtPDF::Annotation::Text())
         << SAP(new QtPDF::Annotation::FreeText())
         << SAP(new QtPDF::Annotation::Caret())
         << SAP(new QtPDF::Annotation::Highlight())
         << SAP(new QtPDF::Annotation::Underline())
         << SAP(new QtPDF::Annotation::Squiggly())
         << SAP(new QtPDF::Annotation::StrikeOut());
  {
    QtPDF::Annotation::Text t;
    t.setName(QStringLiteral("name"));
    annots << SAP(new QtPDF::Annotation::Text(t));
  }
  {
    QtPDF::Annotation::Text t;
    t.setTitle(QStringLiteral("title"));
    annots << SAP(new QtPDF::Annotation::Text(t));
  }
  {
    QtPDF::Annotation::Text t;
    t.setPopup(new QtPDF::Annotation::Popup);
    annots << SAP(new QtPDF::Annotation::Text(t));
  }
  {
    QtPDF::Annotation::Text t;
    t.setPopup(new QtPDF::Annotation::Popup);
    t.popup()->setOpen();
    annots << SAP(new QtPDF::Annotation::Text(t));
  }
  {
    QtPDF::Annotation::Link l;
    l.setHighlightingMode(QtPDF::Annotation::Link::HighlightingInvert);
    annots << SAP(new QtPDF::Annotation::Link(l));
  }
  {
    QtPDF::Annotation::Link l;
    l.setActionOnActivation(QtPDF::PDFGotoAction(QtPDF::PDFDestination(0)).clone());
    annots << SAP(new QtPDF::Annotation::Link(l));
  }
  {
    QtPDF::Annotation::Link l;
    l.setActionOnActivation(QtPDF::PDFGotoAction(QtPDF::PDFDestination(1)).clone());
    annots << SAP(new QtPDF::Annotation::Link(l));
  }
  {
    QtPDF::Annotation::Popup p;
    p.setOpen();
    annots << SAP(new QtPDF::Annotation::Popup(p));
  }

  for (int i = 0; i < annots.size(); ++i) {
    for (int j = 0; j < annots.size(); ++j) {
      if (i == j) {
        QCOMPARE(*annots[i], *annots[j]);
      }
      else {
        QVERIFY2(!(*annots[i] == *annots[j]), qPrintable(QStringLiteral("annots[%1] == annots[%2]").arg(i).arg(j)));
      }
    }
  }
}


void TestQtPDF::page_renderToImage_data()
{
  QTest::addColumn<pDoc>("doc");
  QTest::addColumn<int>("iPage");
  QTest::addColumn<QString>("filename");
  QTest::addColumn<QRect>("rect");
  QTest::addColumn<double>("threshold");
  // Use a higher threshold for the base14 test since the fonts are not
  // embedded. Consequently, system fonts are used as replacements, but they can
  // differ substantially between different systems.

  const pDoc & base14Doc = _docs[QStringLiteral("base14-fonts")];

  QTest::newRow("base14-fonts-ZapfDingbats") << base14Doc << 0 << "base14-fonts-1.png" << QRect(200, 1468, 840, 30) << 150.;
  newDocTest("base14-fonts") << 0 << "base14-fonts-1.png" << QRect() << 15.;
  newDocTest("poppler-data") << 0 << "poppler-data-1.png" << QRect() << 3.;
  newDocTest("jpg") << 0 << "jpg.png" << QRect() << 15.;
}

void TestQtPDF::page_renderToImage()
{
  QFETCH(pDoc, doc);
  QFETCH(int, iPage);
  QFETCH(QString, filename);
  QFETCH(QRect, rect);
  QFETCH(double, threshold);

  QSharedPointer<QtPDF::Backend::Page> page = doc->page(iPage).toStrongRef();
  QVERIFY(page);
  ComparableImage render(page->renderToImage(150, 150).copy(rect), threshold);
//  render.save(filename);
  ComparableImage ref(QImage(filename).copy(rect));
  // Check if the images are both homogeneous (or both not homogeneous)
  // This is intended to catch problems such in base14 tests in which the exact
  // rendering is not tremendously important (hence the threshold is high), but
  // it is crucial that _something_ is rendered (which is not the case, e.g., if
  // a font is missing)
  QCOMPARE(ref.isHomogeneous(), render.isHomogeneous());
  QVERIFY(render == ref);
}

void TestQtPDF::page_loadLinks_data()
{
  QTest::addColumn<pPage>("page");
  QTest::addColumn< QList<QtPDF::Annotation::Link> >("links");

  newPageTest("base14-fonts", 0) << QList<QtPDF::Annotation::Link>();

  {
    QList<QtPDF::Annotation::Link> data;
    QtPDF::Annotation::Link l;
    QRectF r;

    l = QtPDF::Annotation::Link();
    r = QRectF(QPointF(102, 500.89), QPointF(276, 513.89));
    l.setRect(r);
    l.setQuadPoints(QPolygonF(r));
    l.setActionOnActivation(new QtPDF::PDFURIAction(QUrl(QString::fromLatin1("http://www.tug.org/texworks/"))));
    data << l;

    l = QtPDF::Annotation::Link();
    r = QRectF(QPointF(142, 488.89), QPointF(159, 498.89));
    l.setRect(r);
    l.setQuadPoints(QPolygonF(r));
    QtPDF::PDFDestination d(1);
    d.setRect(QRectF(103, 712.89, -1, -1));
    l.setActionOnActivation(new QtPDF::PDFGotoAction(d));
    data << l;

    newPageTest("annotations", 0) << data;
  }

}

void TestQtPDF::page_loadLinks()
{
  QFETCH(pPage, page);
  QFETCH(QList<QtPDF::Annotation::Link>, links);

  QTEST_ASSERT(page);

  QList< QSharedPointer<QtPDF::Annotation::Link> > actual = page->loadLinks();

  QCOMPARE(actual.size(), links.size());

  for (int i = 0; i < actual.size(); ++i) {
    QCOMPARE(*(actual[i]), links[i]);

#ifdef DEBUG
    if (QTest::currentTestFailed()) {
      qDebug() << "Actual Link" << (i + 1);
      qDebug() << "  " << actual[i]->quadPoints();
      if (actual[i]->actionOnActivation())
        qDebug() << "  " << *(actual[i]->actionOnActivation());
      qDebug() << "Expected Link" << (i + 1);
      qDebug() << "  " << links[i].quadPoints();
      if (actual[i]->actionOnActivation())
        qDebug() << "  " << *(links[i].actionOnActivation());
      break;
    }
#endif // defined(DEBUG)
  }
}

// static
void TestQtPDF::compareAnnotation(const QtPDF::Annotation::AbstractAnnotation & a, const QtPDF::Annotation::AbstractAnnotation & b)
{
  int pageA = (a.page().toStrongRef() ? a.page().toStrongRef()->pageNum() : -1);
  int pageB = (b.page().toStrongRef() ? b.page().toStrongRef()->pageNum() : -1);

  QCOMPARE(a.type(), b.type());
  QCOMPARE(pageA, pageB);
  QCOMPARE(a.rect(), b.rect());
  QCOMPARE(a.contents(), b.contents());
}

// static
void TestQtPDF::compareAnnotations(const QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > & a, const QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > & b)
{
  QCOMPARE(a.size(), b.size());

  for (int i = 0; i < a.size(); ++i) {
    compareAnnotation(*(a[i]), *(b[i]));
    if (QTest::currentTestFailed()) return;
  }
}

// static
void TestQtPDF::printAnnotation(const QtPDF::Annotation::AbstractAnnotation & a)
{
  int page = (a.page().toStrongRef() ? a.page().toStrongRef()->pageNum() : -1);
  qDebug() << "   Type:" << a.type() << "page:" << page << "rect:" << a.rect() << "contents:" << a.contents();
}

#define NEW_ANNOT(TYPE, a, r, l) QtPDF::Annotation::TYPE * a = new QtPDF::Annotation::TYPE(); \
a->setPage(page); \
a->setRect(r); \
l << QSharedPointer<QtPDF::Annotation::AbstractAnnotation>(a);


void TestQtPDF::page_loadAnnotations_data()
{
  QTest::addColumn<pPage>("page");
  QTest::addColumn< QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > >("annotations");

  newPageTest("base14-fonts", 0) << QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> >();
  {
    QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > l;
    QWeakPointer<QtPDF::Backend::Page> page = _docs[QString::fromLatin1("annotations")]->page(0);

    {
      NEW_ANNOT(Text, a, QRectF(518.824, 701.148, 14.446, 14.445), l)
      a->setContents(QString::fromLatin1("Comment"));
    }
    { NEW_ANNOT(Highlight, a, QRectF(120.817, 686.702, 0, 0), l) }
    { NEW_ANNOT(Underline, a, QRectF(172.2, 686.702, 0, 0), l) }
    { NEW_ANNOT(Squiggly, a, QRectF(225.843, 686.702, 0, 0), l) }
    { NEW_ANNOT(StrikeOut, a, QRectF(273.329, 686.702, 0, 0), l) }
    {
      NEW_ANNOT(Text, a, QRectF(120.817, 643.364, 113.386, 28.347), l)
      a->setContents(QString::fromLatin1("Free text"));
    }
    {
      NEW_ANNOT(Text, a, QRectF(262.549, 628.918, 113.386, 28.347), l)
      a->setContents(QString::fromLatin1("Free text"));
    }
    {
      NEW_ANNOT(Text, a, QRectF(404.281, 614.473, 113.386, 28.346), l)
      a->setContents(QString::fromLatin1("Free text"));
    }
    // FIXME: annotations.pdf contains more annotations (e.g., line, square,
    // circle) but they are currently not supported by QtPDF
    newPageTest("annotations", 0) << l;
  }
}

void TestQtPDF::page_loadAnnotations()
{
  QFETCH(pPage, page);
  QFETCH(QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> >, annotations);

  QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > actual;

  actual = page->loadAnnotations();
  compareAnnotations(actual, annotations);
  if (QTest::currentTestFailed()) {
    qDebug() << "Actual:";
    for (int i = 0; i < actual.size(); ++i)
      printAnnotation(*(actual[i]));
    qDebug() << "Expected:";
    for (int i = 0; i < annotations.size(); ++i)
      printAnnotation(*(annotations[i]));
  }
}

void TestQtPDF::page_boxes_data()
{
  QTest::addColumn<pPage>("page");
  QTest::addColumn<int>("numBoxes");
  QTest::addColumn<int>("iBox");
  QTest::addColumn<QRectF>("bbox");

  newPageTest("annotations", 1) << 6 << 0 << QRectF(102.884, 124.43037059999996, 20.644181800000013, 20.45768120000001);
}

void TestQtPDF::page_boxes()
{
  QFETCH(pPage, page);
  QFETCH(int, numBoxes);
  QFETCH(int, iBox);
  QFETCH(QRectF, bbox);

  QList< QtPDF::Backend::Page::Box > boxes = page->boxes();

  QCOMPARE(boxes.size(), numBoxes);
  if (iBox >= 0)
    QCOMPARE(boxes[iBox].boundingBox, bbox);
}

void TestQtPDF::page_selectedText_data()
{
  QTest::addColumn<pPage>("page");
  QTest::addColumn< QList<QPolygonF> >("selection");
  QTest::addColumn< QString >("text");

  newPageTest("base14-fonts", 0) << QList<QPolygonF>() << QString();
  newPageTest("base14-fonts", 0) << (QList<QPolygonF>() << QPolygonF(QRectF(0, 105, 400, 10))) << QString::fromLatin1("The quick brown fox jumps over the lazy dog");
  newPageTest("base14-fonts", 0) << (QList<QPolygonF>() << QPolygonF(QRectF(0, 100, 400, 15))) << QString::fromLatin1("Times-Roman\nThe quick brown fox jumps over the lazy dog");
  newPageTest("poppler-data", 0) << (QList<QPolygonF>() << QPolygonF(QRectF(0, 75, 95, 15))) << QString::fromUtf8("香港交易");
}

void TestQtPDF::page_selectedText()
{
  QFETCH(pPage, page);
  QFETCH(QList<QPolygonF>, selection);
  QFETCH(QString, text);

  QCOMPARE(page->selectedText(selection), text);
}

// static
void TestQtPDF::compareSearchResults(const QList<QtPDF::Backend::SearchResult> & actual, const QList<QtPDF::Backend::SearchResult> & expected)
{
  QCOMPARE(actual.size(), expected.size());
  for (int i = 0; i < actual.size(); ++i) {
    QCOMPARE(actual[i].pageNum, expected[i].pageNum);
    QCOMPARE(actual[i].bbox, expected[i].bbox);
  }
}

void TestQtPDF::page_search_data()
{
  QTest::addColumn<pPage>("page");
  QTest::addColumn<QString>("needle");
  QTest::addColumn< QList<QtPDF::Backend::SearchResult> >("results");
  QTest::addColumn<QtPDF::Backend::SearchFlags>("flags");

  {
    QList<QtPDF::Backend::SearchResult> l;
    QtPDF::Backend::SearchResult r;
    r.pageNum = 0;
    r.bbox = QRectF(103, 91.804, 68.664, 10.8);
    l << r;

    newPageTest("base14-fonts", 0) << QString::fromLatin1("Times-Roman") << l << QtPDF::Backend::SearchFlags();
    newPageTest("base14-fonts", 0) << QString::fromLatin1("times-Roman") << QList<QtPDF::Backend::SearchResult>() << QtPDF::Backend::SearchFlags();
    newPageTest("base14-fonts", 0) << QString::fromLatin1("times-Roman") << l << QtPDF::Backend::SearchFlags(QtPDF::Backend::Search_CaseInsensitive);
  }
}

void TestQtPDF::page_search()
{
  QFETCH(pPage, page);
  QFETCH(QString, needle);
  QFETCH(QList<QtPDF::Backend::SearchResult>, results);
  QFETCH(QtPDF::Backend::SearchFlags, flags);

  QList<QtPDF::Backend::SearchResult> actual = page->search(needle, flags);
  compareSearchResults(actual, results);

  if (QTest::currentTestFailed()) {
    qDebug() << "Actual:";
    for (int i = 0; i < actual.size(); ++i)
      qDebug() << "  page:" << (actual[i].pageNum + 1) << "bbox:" << actual[i].bbox;
    qDebug() << "Expected:";
    for (int i = 0; i < results.size(); ++i)
      qDebug() << "  page:" << (results[i].pageNum + 1) << "bbox:" << results[i].bbox;
  }
}

void TestQtPDF::paperSize_data()
{
  QTest::addColumn<QSizeF>("requestSize");
  QTest::addColumn<int>("sizeType");
  QTest::addColumn<QString>("label");
  QTest::addColumn<bool>("landscape");

  QTest::newRow("A4 [mm]") << QSizeF(210, 297) << 0 << QString::fromUtf8("DIN A4 [210 × 297 mm]") << false;
  QTest::newRow("A4 [mm approx]") << QSizeF(209, 298) << 0 << QString::fromUtf8("DIN A4 [210 × 297 mm]") << false;
  QTest::newRow("A4 [mm landscape]") << QSizeF(297, 210) << 0 << QString::fromUtf8("DIN A4 [297 × 210 mm]") << true;
  QTest::newRow("A4 [mm landscape approx]") << QSizeF(295, 212) << 0 << QString::fromUtf8("DIN A4 [297 × 210 mm]") << true;
  QTest::newRow("not A4") << QSizeF(210, 294) << 0 << QString::fromUtf8("210 × 294 mm") << false;

  QTest::newRow("A4 [in]") << QSizeF(8.27, 11.7) << 1 << QString::fromUtf8("DIN A4 [210 × 297 mm]") << false;

  QTest::newRow("A4 [pdf]") << QSizeF(595, 842) << 2 << QString::fromUtf8("DIN A4 [210 × 297 mm]") << false;

  QTest::newRow("Letter") << QSizeF(216, 279) << 0 << QString::fromUtf8("Letter (ANSI A) [8.5 × 11 in]") << false;
}

void TestQtPDF::paperSize()
{
  QFETCH(QSizeF, requestSize);
  QFETCH(int, sizeType);
  QFETCH(QString, label);
  QFETCH(bool, landscape);

  QtPDF::PaperSize ps(QString(), QSizeF(0, 0));

  switch (sizeType) {
  case 0:
  default:
    ps = QtPDF::PaperSize::findForMillimeter(requestSize);
    break;
  case 1:
    ps = QtPDF::PaperSize::findForInch(requestSize);
    break;
  case 2:
    ps = QtPDF::PaperSize::findForPDFSize(requestSize);
    break;
  }

  QCOMPARE(ps.label(), label);
  QCOMPARE(ps.landscape(), landscape);
}

void TestQtPDF::transitions_data()
{
#ifdef Q_OS_MACOS
  // Use longer duration on Mac OS as timing seems flaky there (QTBUG-84998)
  constexpr double duration = 0.2;
#else
  constexpr double duration = 0.05;
#endif
  constexpr int w = 10;
  constexpr int h = 10;
  using SPT = QSharedPointer<QtPDF::Transition::AbstractTransition>;
  QTest::addColumn<QtPDF::Transition::AbstractTransition::Type>("type");
  QTest::addColumn<SPT>("transition");
  QTest::addColumn<double>("duration");
  QTest::addColumn<int>("direction");
  QTest::addColumn<QtPDF::Transition::AbstractTransition::Motion>("motion");
  QTest::addColumn<QImage>("imgStart");
  QTest::addColumn<QImage>("imgEnd");

  QImage imgStart{QSize{w, h}, QImage::Format_ARGB32};
  QImage imgEnd{QSize{w, h}, QImage::Format_ARGB32};

  imgStart.fill(Qt::red);
  imgEnd.fill(Qt::blue);

  QTest::newRow("replace") << QtPDF::Transition::AbstractTransition::Type_Replace << SPT(new QtPDF::Transition::Replace) << duration << -1 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("split-H-in") << QtPDF::Transition::AbstractTransition::Type_Split << SPT(new QtPDF::Transition::Split) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("split-V-in") << QtPDF::Transition::AbstractTransition::Type_Split << SPT(new QtPDF::Transition::Split) << duration << 90 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("split-H-out") << QtPDF::Transition::AbstractTransition::Type_Split << SPT(new QtPDF::Transition::Split) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Outward << imgStart << imgEnd;
  QTest::newRow("split-V-out") << QtPDF::Transition::AbstractTransition::Type_Split << SPT(new QtPDF::Transition::Split) << duration << 90 << QtPDF::Transition::AbstractTransition::Motion_Outward << imgStart << imgEnd;
  QTest::newRow("blinds-H") << QtPDF::Transition::AbstractTransition::Type_Blinds << SPT(new QtPDF::Transition::Blinds) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("blinds-V") << QtPDF::Transition::AbstractTransition::Type_Blinds << SPT(new QtPDF::Transition::Blinds) << duration << 90 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("box-in") << QtPDF::Transition::AbstractTransition::Type_Box << SPT(new QtPDF::Transition::Box) << duration << -1 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("box-out") << QtPDF::Transition::AbstractTransition::Type_Box << SPT(new QtPDF::Transition::Box) << duration << -1 << QtPDF::Transition::AbstractTransition::Motion_Outward << imgStart << imgEnd;
  QTest::newRow("wipe-0") << QtPDF::Transition::AbstractTransition::Type_Wipe << SPT(new QtPDF::Transition::Wipe) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("wipe-90") << QtPDF::Transition::AbstractTransition::Type_Wipe << SPT(new QtPDF::Transition::Wipe) << duration << 90 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("wipe-180") << QtPDF::Transition::AbstractTransition::Type_Wipe << SPT(new QtPDF::Transition::Wipe) << duration << 180 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("wipe-270") << QtPDF::Transition::AbstractTransition::Type_Wipe << SPT(new QtPDF::Transition::Wipe) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("dissolve") << QtPDF::Transition::AbstractTransition::Type_Dissolve << SPT(new QtPDF::Transition::Dissolve) << duration << -1 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("glitter-H") << QtPDF::Transition::AbstractTransition::Type_Glitter << SPT(new QtPDF::Transition::Glitter) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("glitter-V") << QtPDF::Transition::AbstractTransition::Type_Glitter << SPT(new QtPDF::Transition::Glitter) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("glitter-D") << QtPDF::Transition::AbstractTransition::Type_Glitter << SPT(new QtPDF::Transition::Glitter) << duration << 315 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("fly-H-in") << QtPDF::Transition::AbstractTransition::Type_Fly << SPT(new QtPDF::Transition::Fly) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("fly-V-in") << QtPDF::Transition::AbstractTransition::Type_Fly << SPT(new QtPDF::Transition::Fly) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("fly-H-out") << QtPDF::Transition::AbstractTransition::Type_Fly << SPT(new QtPDF::Transition::Fly) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Outward << imgStart << imgEnd;
  QTest::newRow("fly-V-out") << QtPDF::Transition::AbstractTransition::Type_Fly << SPT(new QtPDF::Transition::Fly) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Outward << imgStart << imgEnd;
  QTest::newRow("push-H") << QtPDF::Transition::AbstractTransition::Type_Push << SPT(new QtPDF::Transition::Push) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("push-V") << QtPDF::Transition::AbstractTransition::Type_Push << SPT(new QtPDF::Transition::Push) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("cover-H") << QtPDF::Transition::AbstractTransition::Type_Cover << SPT(new QtPDF::Transition::Cover) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("cover-V") << QtPDF::Transition::AbstractTransition::Type_Cover << SPT(new QtPDF::Transition::Cover) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("uncover-H") << QtPDF::Transition::AbstractTransition::Type_Uncover << SPT(new QtPDF::Transition::Uncover) << duration << 0 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("uncover-V") << QtPDF::Transition::AbstractTransition::Type_Uncover << SPT(new QtPDF::Transition::Uncover) << duration << 270 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
  QTest::newRow("fade") << QtPDF::Transition::AbstractTransition::Type_Fade << SPT(new QtPDF::Transition::Fade) << duration << -1 << QtPDF::Transition::AbstractTransition::Motion_Inward << imgStart << imgEnd;
}

void TestQtPDF::transitions()
{
  QFETCH(QtPDF::Transition::AbstractTransition::Type, type);
  QFETCH(QSharedPointer<QtPDF::Transition::AbstractTransition>, transition);
  QFETCH(double, duration);
  QFETCH(int, direction);
  QFETCH(QtPDF::Transition::AbstractTransition::Motion, motion);
  QFETCH(QImage, imgStart);
  QFETCH(QImage, imgEnd);

  ComparableImage cStart{imgStart};
  ComparableImage cEnd{imgEnd};

  // Test defaults
  QCOMPARE(transition->duration(), 1.0);
  QCOMPARE(transition->direction(), 0);
  QCOMPARE(transition->motion(), QtPDF::Transition::AbstractTransition::Motion_Inward);
  QCOMPARE(transition->isRunning(), false);
  QCOMPARE(transition->isFinished(), false);
  QCOMPARE(transition->getImage().isNull(), true);

  // Set and test values
  transition->setDuration(duration);
  transition->setDirection(direction);
  transition->setMotion(motion);
  QCOMPARE(transition->duration(), duration);
  QCOMPARE(transition->direction(), direction);
  QCOMPARE(transition->motion(), motion);

  // Run animation
  transition->start(imgStart, imgEnd);
  QCOMPARE(transition->isRunning(), true);
  QCOMPARE(transition->isFinished(), false);

  sleep(qRound(0.5 * duration * 1000));
  switch (type) {
    case QtPDF::Transition::AbstractTransition::Type_Replace:
      // Replace directly jumps to the final image
      QVERIFY(ComparableImage(transition->getImage()) == cEnd);
      QCOMPARE(transition->isRunning(), false);
      QCOMPARE(transition->isFinished(), true);
      break;
  case QtPDF::Transition::AbstractTransition::Type_Blinds:
  case QtPDF::Transition::AbstractTransition::Type_Box:
  case QtPDF::Transition::AbstractTransition::Type_Cover:
  case QtPDF::Transition::AbstractTransition::Type_Dissolve:
  case QtPDF::Transition::AbstractTransition::Type_Fade:
  case QtPDF::Transition::AbstractTransition::Type_Fly:
  case QtPDF::Transition::AbstractTransition::Type_Glitter:
  case QtPDF::Transition::AbstractTransition::Type_Push:
  case QtPDF::Transition::AbstractTransition::Type_Split:
  case QtPDF::Transition::AbstractTransition::Type_Uncover:
  case QtPDF::Transition::AbstractTransition::Type_Wipe:
    // Test that we get neither the start nor the end image, i.e., we are
    // "somewhere in the middle"
    // TODO: Potentially test specifics, e.g., that a horizontal swipe is
    // really horizontal
    QVERIFY(ComparableImage(transition->getImage()) != cStart);
    QVERIFY(ComparableImage(transition->getImage()) != cEnd);
    QCOMPARE(transition->isRunning(), true);
    QCOMPARE(transition->isFinished(), false);
    break;
  }

  // Wait slightly longer than 0.5 * duration to ensure t>1 in
  // AbstractTransition::getFracTime()
  sleep(qCeil(0.6 * duration * 1000));

  // Test getImage() before isRunning() as the running state is only updated
  // when getImage() is called
  QVERIFY(ComparableImage(transition->getImage()) == cEnd);
  QCOMPARE(transition->isRunning(), false);
  QCOMPARE(transition->isFinished(), true);

  // Check that the image returned when finished is still the same
  QVERIFY(ComparableImage(transition->getImage()) == cEnd);

  // Reset
  transition->reset();
  QCOMPARE(transition->isRunning(), false);
  QCOMPARE(transition->isFinished(), false);
  // Don't test getImage here - it corresponds to the first frame of the
  // animation, not imgStart
}

void TestQtPDF::pageTile()
{
  QList<QtPDF::Backend::PDFPageTile> tiles;

  const QtPDF::Backend::Document * doc1 = _docs[QStringLiteral("page-rotation")].data();
  const QtPDF::Backend::Document * doc2 = _docs[QStringLiteral("base14-fonts")].data();
  tiles.append({1., 1., QRect(0, 0, 1, 1), doc1, 0});
  tiles.append({2., 1., QRect(0, 0, 1, 1), doc1, 0});
  tiles.append({1., 3., QRect(0, 0, 1, 1), doc1, 0});
  tiles.append({1., 1., QRect(4, 0, 1, 1), doc1, 0});
  tiles.append({1., 1., QRect(0, 5, 1, 1), doc1, 0});
  tiles.append({1., 1., QRect(0, 0, 6, 1), doc1, 0});
  tiles.append({1., 1., QRect(0, 0, 1, 7), doc1, 0});
  tiles.append({1., 1., QRect(0, 0, 1, 1), doc1, 8});
  tiles.append({1., 1., QRect(0, 0, 1, 1), doc2, 0});

  for (int i = 0; i < tiles.size(); ++i) {
    for (int j = i + 1; j < tiles.size(); ++j) {
      auto t1 = tiles[i];
      auto t2 = tiles[j];
      auto h1 = qHash(t1);
      auto h2 = qHash(t2);

      QVERIFY(t1 == t1);
      QVERIFY(t2 == t2);
      QCOMPARE(t1 == t2, false);
      QVERIFY(h1 != h2);

      QCOMPARE(t1 < t1, false);
      QCOMPARE(t2 < t2, false);
      QCOMPARE(t1 < t2 || t2 < t1, true);
    }
  }

#ifdef DEBUG
  QCOMPARE(static_cast<QString>(tiles[0]), QStringLiteral("p0,1x1,r0|0x1|1"));
#endif
}

void TestQtPDF::physicalLength()
{
  using namespace QtPDF::Physical;
  Length l1(1, Length::Inches);

  QCOMPARE(l1.val(Length::Bigpoints), 72.);
  QCOMPARE(l1.val(Length::Inches), 1.);
  QCOMPARE(l1.val(Length::Centimeters), 2.54);

  l1.setVal(144., Length::Bigpoints);
  QCOMPARE(l1.val(Length::Bigpoints), 144.);
  QCOMPARE(l1.val(Length::Inches), 2.);
  QCOMPARE(l1.val(Length::Centimeters), 2 * 2.54);

  QCOMPARE(Length::convert(1, Length::Centimeters, Length::Inches), 1. / 2.54);
}

} // namespace UnitTest

#if defined(STATIC_QT5) && defined(Q_OS_WIN)
  Q_IMPORT_PLUGIN(QWindowsIntegrationPlugin)
#endif

QTEST_MAIN(UnitTest::TestQtPDF)
//#include "TestQtPDF.moc"
