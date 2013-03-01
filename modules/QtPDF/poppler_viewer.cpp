#include <poppler/qt4/poppler-qt4.h>
#include <QtGui/QtGui>

#include <iostream>

int main(int argc, char **argv) {
  QApplication app(argc, argv);

  const double dpiX = app.desktop()->physicalDpiX();
  const double dpiY = app.desktop()->physicalDpiY();

  QMainWindow *mainWin = new QMainWindow();
  QGraphicsView *viewport = new QGraphicsView(mainWin);
  QGraphicsScene *canvas = new QGraphicsScene(viewport);

  viewport->setBackgroundBrush(QBrush(QColor("grey")));

  Poppler::Document *doc = Poppler::Document::load(QString("pgfmanual.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  int i;
  float offY = 0.0;
  QImage pageImage;
  QGraphicsItem *pagePtr;

  pageImage = doc->page(0)->renderToImage(dpiX, dpiY);
  pagePtr = canvas->addPixmap(QPixmap::fromImage(pageImage));

  pagePtr->setPos(0.0, offY);

  QSizeF pageSize, newSize;

  QPixmap placeHolder;
  for (i = 1; i < doc->numPages(); ++i) {
    offY += pageImage.height() + 10.0;

    newSize = doc->page(i)->pageSizeF();
    if ( pageSize != newSize ) {
      std::cerr << "creating new temp page" << std::endl;

      pageSize = newSize;
      pageSize *= 72.0;

      pageSize.setHeight(pageSize.height() / dpiY);
      pageSize.setWidth(pageSize.width() / dpiX);

      placeHolder = QPixmap(pageSize.toSize());
      placeHolder.fill();

    }

    pagePtr = canvas->addPixmap(placeHolder);
    pagePtr->setPos(0.0, offY);
  }

  pagePtr = canvas->items()[0];

  viewport->setScene(canvas);
  viewport->centerOn(pagePtr->boundingRect().center());

  mainWin->setCentralWidget(viewport);
  mainWin->show();
  return app.exec();
  //return 0;
}
