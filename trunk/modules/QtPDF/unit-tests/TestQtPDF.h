#include <QtTest/QtTest>
#include <QObject>

#include "PDFBackend.h"

class TestQtPDF : public QObject
{
  Q_OBJECT

  typedef QSharedPointer<QtPDF::Backend::Document> pDoc;
  QMap<QString, pDoc> _docs;

  QTestData & newDocTest(const char * tag);

private slots:
  void loadDocs();
//  virtual void reload() = 0;

  void isValid_data();
  void isValid();

  void isLocked_data();
  void isLocked();

  void numPages_data();
  void numPages();

  void fileName_data();
  void fileName();

  void page_data();
  void page();
  // void resolveDestination();

  void metaDataTitle_data();
  void metaDataTitle();

  void metaDataAuthor_data();
  void metaDataAuthor();

  void metaDataSubject_data();
  void metaDataSubject();

  void metaDataKeywords_data();
  void metaDataKeywords();

  void metaDataCreator_data();
  void metaDataCreator();

  void metaDataProducer_data();
  void metaDataProducer();

  void metaDataCreationDate_data();
  void metaDataCreationDate();

  void metaDataModDate_data();
  void metaDataModDate();

  void metaDataTrapped_data();
  void metaDataTrapped();


/*
  void permissions();
  virtual bool isValid() const = 0;
  // Uses doc-read-lock
  virtual bool isLocked() const = 0;
  virtual bool unlock(const QString password) = 0;
  virtual PDFToC toc() const { return PDFToC(); }
  virtual QList<PDFFontInfo> fonts() const { return QList<PDFFontInfo>(); }
  QMap<QString, QString> metaDataOther() const { QReadLocker docLocker(_docLock.data()); return _meta_other; }
  virtual QList<SearchResult> search(QString searchText, int startPage=0);
*/
};

Q_DECLARE_METATYPE(QSharedPointer<QtPDF::Backend::Document>)
