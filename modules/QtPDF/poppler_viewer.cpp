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

  Poppler::Document *doc = Poppler::Document::load(QString("luatex.pdf"));
  std::cerr << "number of pages: " << doc->numPages() << std::endl;

  int i;
  float offY = 0.0;
  QImage pageImage;
  QGraphicsItem *pagePtr;
  for (i = 0; i < 10; ++i) {
    pageImage = doc->page(i)->renderToImage(dpiX, dpiY);
    pagePtr = canvas->addPixmap(QPixmap::fromImage(pageImage));

    pagePtr->setPos(0.0, offY);
    offY += pageImage.height() + 10.0;
  }

  pagePtr = canvas->items()[0];

  viewport->setScene(canvas);
  viewport->centerOn(pagePtr->boundingRect().center());

  mainWin->setCentralWidget(viewport);
  mainWin->show();
  return app.exec();
}
