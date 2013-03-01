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


int main(int argc, char **argv) {
  QApplication app(argc, argv);

  QMainWindow mainWin;
  QGraphicsView *viewport = new QGraphicsView(&mainWin);
  QGraphicsScene *canvas = new QGraphicsScene(viewport);

  viewport->setBackgroundRole(QPalette::Dark);
  viewport->setAlignment(Qt::AlignCenter);

  Poppler::Document *doc = Poppler::Document::load(QString("pgfmanual.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  int i;
  float offY = 0.0;
  PDFPageGraphicsItem *pagePtr;

  for (i = 0; i < doc->numPages(); ++i) {
    pagePtr = new PDFPageGraphicsItem(doc->page(i));
    pagePtr->setPos(0.0, offY);

    canvas->addItem(pagePtr);
    offY += pagePtr->pixmap().height() + 10.0;
  }

  viewport->setScene(canvas);

  QGraphicsItem *firstPage = viewport->items()[0];
  viewport->centerOn(firstPage->boundingRect().center());

  mainWin.setCentralWidget(viewport);
  mainWin.show();
  return app.exec();
  //return 0;
}
