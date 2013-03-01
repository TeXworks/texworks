#include <QtGui/QtGui>

class PDFViewer : public QMainWindow {
  Q_OBJECT

public:
  PDFViewer(QString pdf_doc, QWidget *parent = 0, Qt::WindowFlags flags = 0);

private slots:
  void openUrl(const QUrl url) const;
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
