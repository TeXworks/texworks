/*
  This is part of TeXworks, an environment for working with TeX documents
  Copyright (C) 2013-2020  Stefan Löffler

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

#include "PDFBackend.h"
#include "PDFTransitions.h"

#include <QObject>
#include <QtTest/QtTest>

namespace UnitTest {

class TestQtPDF : public QObject
{
  Q_OBJECT

  typedef QSharedPointer<QtPDF::Backend::Document> pDoc;
  typedef QSharedPointer<QtPDF::Backend::Page> pPage;
  QMap<QString, pDoc> _docs;

  QTestData & newDocTest(const char * tag);
  QTestData & newPageTest(const char * tag, const int iPage);

  static void compareAnnotation(const QtPDF::Annotation::AbstractAnnotation & a, const QtPDF::Annotation::AbstractAnnotation & b);
  static void compareAnnotations(const QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > & a, const QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> > & b);
  static void compareLinks(const QtPDF::Annotation::Link & actual, const QtPDF::Annotation::Link & expected);
  static void compareSearchResults(const QList<QtPDF::Backend::SearchResult> & actual, const QList<QtPDF::Backend::SearchResult> & expected);
  static void compareToC(const QtPDF::Backend::PDFToC & actual, const QtPDF::Backend::PDFToC & expected);

  static void printAction(const QtPDF::PDFAction & a);
  static void printAnnotation(const QtPDF::Annotation::AbstractAnnotation & a);
  static void printToC(const QtPDF::Backend::PDFToC & toc, const QString & indent = QString::fromLatin1("  "));

private slots:
  void backendInterface();
  void abstractBaseClasses();

  void loadDocs();
//  virtual void reload() = 0;

  void parsePDFDate_data();
  void parsePDFDate();

  void isValid_data();
  void isValid();

  void isLocked_data();
  void isLocked();

  void unlock_data();
  void unlock();

  void numPages_data();
  void numPages();

  void fileName_data();
  void fileName();

  void page_data();
  void page();

  void destination_data();
  void destination();

  void destinationComparison();

  void PDFUriAction();
  void PDFGotoAction();
  void PDFLaunchAction();
  void actionComparison();

  void resolveDestination_data();
  void resolveDestination();

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

  void metaDataOther_data();
  void metaDataOther();

  void fileSize_data();
  void fileSize();

  void pageSize_data();
  void pageSize();

  void permissions_data();
  void permissions();

  void fontDescriptor_data();
  void fontDescriptor();

  void fontDescriptorComparison();

  void fonts_data();
  void fonts();

  void ToCItem();

  void toc_data();
  void toc();

  void annotationComparison();

  void page_renderToImage_data();
  void page_renderToImage();

  void page_loadLinks_data();
  void page_loadLinks();

  void page_loadAnnotations_data();
  void page_loadAnnotations();

  void page_boxes_data();
  void page_boxes();

  void page_selectedText_data();
  void page_selectedText();

  void page_search_data();
  void page_search();

  void paperSize_data();
  void paperSize();

  void transitions_data();
  void transitions();

  void pageTile();
};

} // namespace UnitTest

typedef QMap<QString, QString> QStringMap;

Q_DECLARE_METATYPE(QSharedPointer<QtPDF::Backend::Document>)
Q_DECLARE_METATYPE(QSharedPointer<QtPDF::Backend::Page>)
Q_DECLARE_METATYPE(QtPDF::Backend::Document::Permissions)
Q_DECLARE_METATYPE(QtPDF::Backend::PDFToC)
Q_DECLARE_METATYPE(QtPDF::PDFDestination)
Q_DECLARE_METATYPE(QList<QtPDF::Annotation::Link>)
Q_DECLARE_METATYPE(QList<QPolygonF>)
Q_DECLARE_METATYPE(QList<QtPDF::Backend::SearchResult>)
Q_DECLARE_METATYPE(QtPDF::Backend::SearchFlags)
Q_DECLARE_METATYPE(QList< QSharedPointer<QtPDF::Annotation::AbstractAnnotation> >)
Q_DECLARE_METATYPE(QStringMap)
Q_DECLARE_METATYPE(QSharedPointer<QtPDF::Transition::AbstractTransition>)
Q_DECLARE_METATYPE(QtPDF::Transition::AbstractTransition::Type)
Q_DECLARE_METATYPE(QtPDF::Transition::AbstractTransition::Motion)
Q_DECLARE_METATYPE(QtPDF::PDFDestination::Type)
