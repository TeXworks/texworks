#include <QtGui/QtGui>
#include "PDFDocumentView.h"

class PageCounter;
class ZoomTracker;
class SearchLineEdit;

class PDFViewer : public QMainWindow {
  Q_OBJECT

  PageCounter * _counter;
  ZoomTracker * _zoomWdgt;
  SearchLineEdit * _search;
  QToolBar * _toolBar;

public:
  PDFViewer(const QString pdf_doc = QString(), QWidget *parent = 0, Qt::WindowFlags flags = 0);

public slots:
  void open();

private slots:
  void openUrl(const QUrl url) const;
  void openPdf(QString filename, int page, bool newWindow) const;
  void syncFromPdf(const int page, const QPointF pos);
  void searchProgressChanged(int percent, int occurrences);
  void documentChanged(const QSharedPointer<QtPDF::Document> newDoc);
  
#ifdef DEBUG
  // FIXME: Remove this
  void setGermanLocale() { emit switchInterfaceLocale(QLocale(QLocale::German)); }
  void setEnglishLocale() { emit switchInterfaceLocale(QLocale(QLocale::C)); }
#endif

signals:
  void switchInterfaceLocale(const QLocale & newLocale);
};


class SearchLineEdit : public QLineEdit
{
  Q_OBJECT

public:
  SearchLineEdit(QWidget *parent = 0);

protected:
  void resizeEvent(QResizeEvent *);

private:
  QToolButton *nextResultButton, *previousResultButton, *clearButton;

signals:
  void searchRequested(QString searchText);
  void gotoNextResult();
  void gotoPreviousResult();
  void searchCleared();

private slots:
  void prepareSearch();
  void clearSearch();
  void handleNextResult();
  void handlePreviousResult();

};


class PageCounter : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  int currentPage, lastPage;

public:
  PageCounter(QWidget *parent = 0, Qt::WindowFlags f = 0);

public slots:
  void setLastPage(int page);
  void setCurrentPage(int page);

private:
  void refreshText();
};


class ZoomTracker : public QLabel {
  Q_OBJECT
  typedef QLabel super;
  qreal zoom;

public:
  ZoomTracker(QWidget *parent = 0, Qt::WindowFlags f = 0);

public slots:
  void setZoom(qreal newZoom);

private:
  void refreshText();
};
