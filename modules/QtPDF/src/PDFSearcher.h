#ifndef PDFSearcher_H
#define PDFSearcher_H

#include "PDFBackend.h"

#include <QObject>
#include <QThread>

namespace QtPDF {

class PDFSearcher : public QThread
{
	Q_OBJECT

  struct SearchResult {
    QList<Backend::SearchResult> occurences;
    bool finished{false};
  };

  QString m_searchString;
  Backend::SearchFlags m_searchFlags;
  int m_startPage{0};
  QVector<SearchResult> m_results;
  QWeakPointer<Backend::Document> m_doc;
  QVector<int> m_pages;
  mutable QMutex m_mutex;

  void populatePages();

protected:
  void stopAndClear();

public:
  void ensureStopped();
  void clear();

  QString searchString() const;
  void setSearchString(const QString & searchString);
  Backend::SearchFlags searchFlags() const;
  void setSearchFlags(const Backend::SearchFlags & flags);
  QWeakPointer<QtPDF::Backend::Document> document() const;
  void setDocument(const QWeakPointer<QtPDF::Backend::Document> & doc);
  int startPage() const;
  void setStartPage(int page);

  int progressValue() const;
  int progressMinimum() const { return 0; }
  int progressMaximum() const;

  QList<Backend::SearchResult> resultAt(int page) const;

signals:
  void resultReady(int page);
  void progressValueChanged(int progressValue);

protected:
  void run() final;
};

} // namespace QtPDF

#endif // !defined(PDFSearcher_H)
