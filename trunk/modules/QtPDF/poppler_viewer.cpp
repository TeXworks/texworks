#include <poppler/qt4/poppler-qt4.h>
#include <QtGui/QtGui>

#include <iostream>

class PDFPageGraphicsItem : public QGraphicsPixmapItem {
  typedef QGraphicsPixmapItem super;
  // To spare the need for a destructor
  const std::auto_ptr<Poppler::Page> page;
  bool dirty;
  double dpiX;
  double dpiY;

public:

  PDFPageGraphicsItem(Poppler::Page *a_page, QGraphicsItem *parent = 0) : super(parent), page(a_page), dirty(true) {
    dpiX = QApplication::desktop()->physicalDpiX();
    dpiY = QApplication::desktop()->physicalDpiY();

    // Create an empty pixmap that is the same size as the PDF page. This
    // allows us to dielay the rendering of pages until they actually come into
    // view which saves time.
    QSizeF pageSize = page->pageSizeF() / 72.0;
    pageSize.setHeight(pageSize.height() * dpiY);
    pageSize.setWidth(pageSize.width() * dpiX);

    setPixmap(QPixmap(pageSize.toSize()));
  }

  void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
    // If this `PDFPageGraphicsItem` is still using the empty `QPixmap` it was
    // constructed with, `dirty` will be `true`. We replace the empty pixmap
    // with a rendered image of the page.
    if ( this->dirty ) {
      // `convertFromImage` was previously part of the Qt3 API and was
      // forward-ported to Qt 4 in version 4.7. Supposidly more efficient than
      // using `QPixmap::fromImage` as it does not involve constructing a new
      // QPixmap.
      //
      // If the performance hit is not outrageous, may want to consider
      // downgrading to `QPixmap::fromImage` in order to support more versions
      // of Qt 4.x.
      this->pixmap().convertFromImage(this->page->renderToImage(dpiX, dpiY));
      this->dirty = false;
    }

    // After checking if page image needs to be rendered, punt call back to the
    // `paint` method of `QGraphicsPixmapItem`.
    super::paint(painter, option, widget);
  }

private:
  // Parent has no copy constructor, so this class shouldn't either. Also, we
  // hold some information in an `auto_ptr` which does interesting things on
  // copy that C++ newbies may not expect.
  Q_DISABLE_COPY(PDFPageGraphicsItem)

};


class PDFDocumentView : public QGraphicsView {
  typedef QGraphicsView super;
  const std::auto_ptr<Poppler::Document> doc;

  // This may change to a `QSet` in the future
  QList<QGraphicsItem*> pages;
  int _currentPage, _lastPage;

public:
  PDFDocumentView(Poppler::Document *a_doc, QWidget *parent = 0) : super(new QGraphicsScene, parent),
    doc(a_doc)
  {
    setBackgroundRole(QPalette::Dark);
    setAlignment(Qt::AlignCenter);
    setFocusPolicy(Qt::StrongFocus);

    // **TODO:** _Investigate the Arthur backent for native Qt rendering._
    doc->setRenderBackend(Poppler::Document::SplashBackend);
    // Make things look pretty.
    doc->setRenderHint(Poppler::Document::Antialiasing);
    doc->setRenderHint(Poppler::Document::TextAntialiasing);

    _lastPage = doc->numPages();

    // Create a `PDFPageGraphicsItem` for each page in the PDF document to the
    // `QGraphicsScene` controlled by this object. The Y-coordinate of each
    // page is automatically shifted such that it will appear 10px below the
    // previous page.
    //
    // **TODO:** _Should the Y-shift be sensitive to zoom levels?_
    int i;
    float offY = 0.0;
    PDFPageGraphicsItem *pagePtr;

    for (i = 0; i < _lastPage; ++i) {
      pagePtr = new PDFPageGraphicsItem(doc->page(i));
      pagePtr->setPos(0.0, offY);

      pages.append(pagePtr);
      scene()->addItem(pagePtr);

      offY += pagePtr->pixmap().height() + 10.0;
    }

    // Automatically center on the first page in the PDF document.
    goToPage(0);
  }

  ~PDFDocumentView() {
    delete this->scene();
  }

  int currentPage() {
    return _currentPage;
  }

  int lastPage() {
    return _lastPage;
  }

  // **TODO:** _Overload this function to take `PDFPageGraphicsItem` as a
  // parameter?_
  void goToPage(int pageNum) {
    // We silently ignore any invalid page numbers.
    if ( (pageNum >= 0) && (pageNum < _lastPage) && (pageNum != _currentPage) ) {
      centerOn(pages.at(pageNum));
      _currentPage = pageNum;
    }
  }


protected:

  // Keep track of the current page by overloading the widget paint event.
  void paintEvent(QPaintEvent *event) {
    super::paintEvent(event);

    // After `QGraphicsView` has taken care of updates to this widget, find the
    // currently displayed page. We do this by grabbing all items that are
    // currently within the bounds of the viewport's top half. We take the
    // first item found to be the "current page".
    //
    // **NOTE:**
    // _If graphics objects other than `PDFPageGraphicsItem` are ever added to
    // the `GraphicsScene` managed by `PDFDocumentView` (such as annotations,
    // form elements, etc), it may be wise to ensure this selection only
    // considers `PDFPagegraphicsItem` objects.
    //
    // A way to do this may be to call `toSet` on both `pages` and the result
    // of `items` and then take the first item of a set intersection.
    QRect pageBbox = viewport()->rect();
    pageBbox.setHeight(0.5 * pageBbox.height());
    int nextCurrentPage = pages.indexOf(items(pageBbox).first());

    if (nextCurrentPage != _currentPage) {
      _currentPage = nextCurrentPage;
      // **TODO:** _Should probably also emit a "Current page changed" event._
    }

  }

  // Very "dumb" attempt at keypress handling.
  //
  // **TODO:**
  //
  //   * _Scroll events need to be captured and `currentPage` adjusted
  //     accordingly._
  //
  //   * _Should pageUp/pageDown be implemented in terms of signals/slots and
  //     let some parent widget worry about delegating Page Up/PageDown/other
  //     keypresses?_
  void keyPressEvent(QKeyEvent *event) {

    if (
      (event->key() == Qt::Key_Home) &&
      (_currentPage != 0)
    ) {

      goToPage(0);
      event->accept();

    } else if(
      (event->key() == Qt::Key_End) &&
      (_currentPage != _lastPage)
    ) {

      goToPage(_lastPage - 1);
      event->accept();

    } else if(
      (event->key() == Qt::Key_PageDown) &&
      (_currentPage < _lastPage)
    ) {

      goToPage(_currentPage + 1);
      event->accept();

    } else if (
      (event->key() == Qt::Key_PageUp) &&
      (_currentPage > 0)
    ) {

      goToPage(_currentPage - 1);
      event->accept();

    } else  {

      super::keyPressEvent(event);

    }

  }

private:
  // Parent class has no copy constructor.
  Q_DISABLE_COPY(PDFDocumentView)

  // **TODO:**
  // _This class basically comes with a built-in `QGraphicsScene` unlike a
  // traditional `QGraphicsView` where the scenes can be swapped around. Should
  // we disable the `setScene` function by declaring it `private`? Does it make
  // sense to have different graphics scenes?_

};


int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QMainWindow mainWin;

  Poppler::Document *doc = Poppler::Document::load(QString("pgfmanual.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  PDFDocumentView *docView = new PDFDocumentView(doc, &mainWin);

  mainWin.setCentralWidget(docView);
  mainWin.show();
  return app.exec();
  //return 0;
}
