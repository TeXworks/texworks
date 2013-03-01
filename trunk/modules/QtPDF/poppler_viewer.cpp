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
  int currentPage, lastPage;

public:
  PDFDocumentView(Poppler::Document *a_doc, QWidget *parent = 0) : super(new QGraphicsScene, parent),
    doc(a_doc)
  {
    setBackgroundRole(QPalette::Dark);
    setAlignment(Qt::AlignCenter);
    setFocusPolicy(Qt::StrongFocus);

    lastPage = doc->numPages();

    // Create a `PDFPageGraphicsItem` for each page in the PDF document to the
    // `QGraphicsScene` controlled by this object. The Y-coordinate of each
    // page is automatically shifted such that it will appear 10px below the
    // previous page.
    //
    // **TODO:** _Should these be packed together into a `QGraphicsItemGroup`
    // for easy access/removal?_
    //
    // **TODO:** _Should the Y-shift be sensitive to zoom levels?_
    int i;
    float offY = 0.0;
    PDFPageGraphicsItem *pagePtr;

    for (i = 0; i < lastPage; ++i) {
      pagePtr = new PDFPageGraphicsItem(doc->page(i));
      scene()->addItem(pagePtr);

      pagePtr->setPos(0.0, offY);
      offY += pagePtr->pixmap().height() + 10.0;
    }

    // Automatically center on the first page in the PDF document.
    //
    // **TODO:** _Should probably be a seperate method. Like `firstPage` or
    // something._
    QGraphicsItem *firstPage = scene()->items(Qt::AscendingOrder).first();
    centerOn(firstPage);
    currentPage = 0;
  }

  ~PDFDocumentView() {
    delete this->scene();
  }

protected:
  // Very "dumb" first cut at pageup/page down handling.
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
    QGraphicsItem *targetPage;

    if(
      (event->key() == Qt::Key_PageDown) &&
      (currentPage < lastPage)
    ) {

      ++currentPage;
      targetPage = scene()->items(Qt::AscendingOrder)[currentPage];
      centerOn(targetPage);
      event->accept();

    } else if (
      (event->key() == Qt::Key_PageUp) &&
      (currentPage > 0)
    ) {

      --currentPage;
      targetPage = scene()->items(Qt::AscendingOrder)[currentPage];
      centerOn(targetPage);
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
