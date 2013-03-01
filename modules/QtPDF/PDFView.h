#include <memory>
#include <QtGui/QtGui>
#include <poppler/qt4/poppler-qt4.h>

class PDFPageGraphicsItem : public QGraphicsPixmapItem {
  typedef QGraphicsPixmapItem super;
  // To spare the need for a destructor
  const std::auto_ptr<Poppler::Page> page;
  bool dirty;
  double dpiX;
  double dpiY;

public:

  PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent = 0);
  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFPageGraphicsItem)
};


class PDFDocumentView : public QGraphicsView {
  Q_OBJECT
  typedef QGraphicsView super;
  const std::auto_ptr<Poppler::Document> doc;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> pages;
  int _currentPage, _lastPage;

public:
  PDFDocumentView(Poppler::Document *a_doc, QWidget *parent = 0);
  ~PDFDocumentView();
  int currentPage();
  int lastPage();
  void goToPage(int pageNum);

signals:
  void changedPage(int);

protected:
  // Keep track of the current page by overloading the widget paint event.
  void paintEvent(QPaintEvent *event);
  void keyPressEvent(QKeyEvent *event);

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFDocumentView)

  // **TODO:**
  // _This class basically comes with a built-in `QGraphicsScene` unlike a
  // traditional `QGraphicsView` where the scenes can be swapped around. Should
  // we disable the `setScene` function by declaring it `private`? Does it make
  // sense to have different graphics scenes?_

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
