/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

#include <PDFBackend.h>
#include <QPainter>
#include <QApplication>

// TODO: Find a better place to put this
QBrush * pageDummyBrush = NULL;

QDateTime fromPDFDate(QString pdfDate)
{
  QDate date;
  QTime time;
  QString format;
  QDateTime retVal;
  int sign = 0;
  int hourOffset, minuteOffset;
  bool ok;

  // "D:" prefix is strongly recommended, but optional; we don't need it here
  if (pdfDate.startsWith(QString::fromUtf8("D:")))
    pdfDate.remove(0, 2);

  // Parse the date
  if (pdfDate.length() < 4)
    return QDateTime();
  format = QString::fromUtf8("yyyy");
  if (pdfDate.length() >= 6)
    format += QString::fromUtf8("MM");
  if (pdfDate.length() >= 8)
    format += QString::fromUtf8("dd");
  date = QDate::fromString(pdfDate.left(format.length()), format);
  pdfDate.remove(0, format.length());

  // Parse the time
  if (pdfDate.length() < 2)
    return QDateTime(date, time);
  format = QString::fromUtf8("hh");
  if (pdfDate.length() >= 4)
    format += QString::fromUtf8("mm");
  if (pdfDate.length() >= 6)
    format += QString::fromUtf8("ss");
  time = QTime::fromString(pdfDate.left(format.length()), format);
  pdfDate.remove(0, format.length());

  // Parse time zone data
  if (pdfDate.length() == 0)
    return QDateTime(date, time);
  switch (pdfDate[0].toAscii()) {
    case 'Z':
      return QDateTime(date, time, Qt::UTC).toLocalTime();
    case '+':
      // Note: A `+` signifies that pdfDate is later than UTC. Since we will
      // specify the QDateTime in UTC below, we have to _subtract_ the offset
      sign = -1;
      break;
    case '-':
      sign = +1;
      break;
    default:
      return QDateTime(date, time);
  }
  pdfDate.remove(0, 1);
  if (pdfDate.length() < 3 || pdfDate[2] != QChar::fromAscii('\''))
    return QDateTime(date, time);
  hourOffset = pdfDate.left(2).toInt(&ok);
  if (!ok)
    return QDateTime(date, time);
  pdfDate.remove(0, 3);
  if (pdfDate.length() >= 3 && pdfDate[2] ==  QChar::fromAscii('\''))
    minuteOffset = pdfDate.left(2).toInt();
  return QDateTime(date, time, Qt::UTC).addSecs(sign * (hourOffset * 3600 + minuteOffset * 60)).toLocalTime();
}





// Fonts
// =================

PDFFontDescriptor::PDFFontDescriptor(const QString fontName /* = QString() */) :
  _name(fontName),
  _stretch(FontStretch_Normal),
  _weight(400),
  _italicAngle(0),
  _ascent(0),
  _descent(0),
  _leading(0),
  _capHeight(0),
  _xHeight(0),
  _stemV(0),
  _stemH(0),
  _avgWidth(0),
  _maxWidth(0),
  _missingWidth(0)
{
}

bool PDFFontDescriptor::isSubset() const
{
  // Subset fonts have a tag of 6 upper-case letters, followed by a '+',
  // prefixed to the font name
  if (_name.length() < 7 || _name[6] != QChar::fromAscii('+'))
    return false;
  for (int i = 0; i < 6; ++i) {
    if (!_name[i].isUpper())
      return false;
  }
  return true;
}

QString PDFFontDescriptor::pureName() const
{
  if (!isSubset())
    return _name;
  else
    return _name.mid(7);
}


// Backend Rendering
// =================
// The `PDFPageProcessingThread` is a thread that processes background jobs.
// Each job is represented by a subclass of `PageProcessingRequest` and
// contains an `execute` method that performs the actual work.
PDFPageProcessingThread::PDFPageProcessingThread() :
_quit(false)
{
}

PDFPageProcessingThread::~PDFPageProcessingThread()
{
  _mutex.lock();
  _quit = true;
  _waitCondition.wakeAll();
  _mutex.unlock();
  wait();
}

void PDFPageProcessingThread::addPageProcessingRequest(PageProcessingRequest * request)
{
  int i;

  if (!request)
    return;

  // `request` must live in the main (GUI) thread, or else destroying it later
  // on will fail
  Q_ASSERT(request->thread() == QApplication::instance()->thread());

  QMutexLocker locker(&(this->_mutex));
  // remove any instances of the given request type before adding the new one to
  // avoid processing it several times
  // **TODO:** Could it be that we require several concurrent versions of the
  //           same page?
  for (i = _workStack.size() - 1; i >= 0; --i) {
    if (*(_workStack[i]) == *request) {
      // Using deleteLater() doesn't work because we have no event queue in this
      // thread. However, since the object is still on the stack, it is still
      // sleeping and directly deleting it should therefore be safe.
      delete _workStack[i];
      _workStack.remove(i);
    }
  }

  _workStack.push(request);
  locker.unlock();
#ifdef DEBUG
  QString jobDesc;
  switch (request->type()) {
    case PageProcessingRequest::LoadLinks:
      {
        qDebug() << "new 'loading links request' for page" << request->page->pageNum();
      }
      break;
    case PageProcessingRequest::PageRendering:
      {
        PageProcessingRenderPageRequest * r = static_cast<PageProcessingRenderPageRequest*>(request);
        qDebug() << "new 'rendering page request' for page" << request->page->pageNum() << "and tile" << r->render_box;
      }
      break;
  }
#endif

  if (!isRunning())
    start();
  else
    _waitCondition.wakeOne();
}

void PDFPageProcessingThread::run()
{
  PageProcessingRequest * workItem;

  _mutex.lock();
  while (!_quit) {
    // mutex must be locked at start of loop
    if (_workStack.size() > 0) {
      workItem = _workStack.pop();
      _mutex.unlock();

#ifdef DEBUG
      qDebug() << "processing work item; remaining items:" << _workStack.size();
      _renderTimer.start();
#endif
      workItem->execute();
#ifdef DEBUG
      QString jobDesc;
      switch (workItem->type()) {
        case PageProcessingRequest::LoadLinks:
          jobDesc = QString::fromUtf8("loading links");
          break;
        case PageProcessingRequest::PageRendering:
          jobDesc = QString::fromUtf8("rendering page");
          break;
      }
      qDebug() << "finished " << jobDesc << "for page" << workItem->page->pageNum() << ". Time elapsed: " << _renderTimer.elapsed() << " ms.";
#endif

      // Delete the work item as it has fulfilled its purpose
      // Note that we can't delete it here or we might risk that some emitted
      // signals are invalidated; to ensure they reach their destination, we
      // need to call deleteLater().
      // Note: workItem *must* live in the main (GUI) thread for this!
      Q_ASSERT(workItem->thread() == QApplication::instance()->thread());
      workItem->deleteLater();

      _mutex.lock();
    }
    else {
#ifdef DEBUG
      qDebug() << "going to sleep";
#endif
      _waitCondition.wait(&_mutex);
#ifdef DEBUG
      qDebug() << "waking up";
#endif
    }
  }
}


// Asynchronous Page Operations
// ----------------------------
//
// The `execute` functions here are called by the processing theread to perform
// background jobs such as page rendering or link loading. This alows the GUI
// thread to stay unblocked and responsive. The results of background jobs are
// posted as events to a `listener` which can be any subclass of `QObject`. The
// `listener` will need a custom `event` function that is capable of picking up
// on these events.

bool PageProcessingRequest::operator==(const PageProcessingRequest & r) const
{
  // TODO: Should we care about the listener here as well?
  return (type() == r.type() && page == r.page);
}

bool PageProcessingRenderPageRequest::operator==(const PageProcessingRequest & r) const
{
  if (r.type() != PageRendering)
    return false;
  const PageProcessingRenderPageRequest * rr = static_cast<const PageProcessingRenderPageRequest*>(&r);
  // TODO: Should we care about the listener here as well?
  return (xres == rr->xres && yres == rr->yres && render_box == rr->render_box && cache == rr->cache);
}

// ### Custom Event Types
// These are the events posted by `execute` functions.
const QEvent::Type PDFPageRenderedEvent::PageRenderedEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );
const QEvent::Type PDFLinksLoadedEvent::LinksLoadedEvent = static_cast<QEvent::Type>( QEvent::registerEventType() );

bool PageProcessingRenderPageRequest::execute()
{
  // FIXME: Aborting renders doesn't really work right now---the backend knows
  // nothing about the PDF scenes.
  //
  // Idea: Perhaps allow page render requests to provide a pointer to a function
  // that returns a `bool` value indicating if the request is still valid? Then
  // the `PDFPageGraphicsItem` could have a function that indicates if the item
  // is anywhere near a viewport.
  QImage rendered_page = page->renderToImage(xres, yres, render_box, cache);
  QCoreApplication::postEvent(listener, new PDFPageRenderedEvent(xres, yres, render_box, rendered_page));

  return true;
}

bool PageProcessingLoadLinksRequest::execute()
{
  QCoreApplication::postEvent(listener, new PDFLinksLoadedEvent(page->loadLinks()));
  return true;
}

// ### Cache for Rendered Images
uint qHash(const PDFPageTile &tile)
{
  // FIXME: This is a horrible, horrible hash function, but it is a good quick and dirty
  // implementation. Should come up with something that executes faster.
  QByteArray hash_string;
  QDataStream(&hash_string, QIODevice::WriteOnly) << tile.xres << tile.yres << tile.render_box << tile.page_num;
  return qHash(hash_string);
}

QSharedPointer<QImage> PDFPageCache::getImage(const PDFPageTile & tile) const
{
  _lock.lockForRead();
  QSharedPointer<QImage> * retVal = object(tile);
  _lock.unlock();
  if (retVal)
    return *retVal;
  return QSharedPointer<QImage>();
}

QSharedPointer<QImage> PDFPageCache::setImage(const PDFPageTile & tile, QImage * image, const bool overwrite /* = true */)
{
  _lock.lockForWrite();
  QSharedPointer<QImage> retVal;
  if (contains(tile))
    retVal = *object(tile);
  // If the key is not in the cache yet add it. Otherwise overwrite the cached
  // image but leave the pointer intact as that can be held/used elsewhere
  if (!retVal) {
    QSharedPointer<QImage> * toInsert = new QSharedPointer<QImage>(image);
    insert(tile, toInsert, (image ? image->byteCount() : 0));
    retVal = *toInsert;
  }
  else if(overwrite) {
    // TODO: overwriting an image with a different one can change its size (and
    // therefore its cost in the cache). There doesn't seem to be a method to
    // hande that in QCache, though, and since we only use one tile size this
    // shouldn't pose a problem.
    if (image)
      *retVal = *image;
    else {
      QSharedPointer<QImage> * toInsert = new QSharedPointer<QImage>;
      insert(tile, toInsert, 0);
      retVal = *toInsert;
    }
  }
  _lock.unlock();
  return retVal;
}


// PDF ABCs
// ========

// Document Class
// --------------
Document::Document(QString fileName):
  _numPages(-1)
{
  // Set cache for rendered pages to be 1GB. This is enough for 256 RGBA tiles
  // (1024 x 1024 pixels x 4 bytes per pixel).
  //
  // NOTE: The application seems to exceed 1 GB---usage plateaus at around 2GB. No idea why. Perhaps freed
  // blocks are not garbage collected?? Perhaps my math is off??
  _pageCache.setMaxSize(1024 * 1024 * 1024);
}

Document::~Document()
{
}

int Document::numPages() { return _numPages; }
PDFPageProcessingThread &Document::processingThread() { return _processingThread; }
PDFPageCache &Document::pageCache() { return _pageCache; }

QList<SearchResult> Document::search(QString searchText, int startPage)
{
  QList<SearchResult> results;
  int i;

  for (i = startPage; i < _numPages; ++i)
    results << page(i)->search(searchText);
  for (i = 0; i < startPage; ++i)
    results << page(i)->search(searchText);


  return results;
}


// Page Class
// ----------
Page::Page(Document *parent, int at):
  _parent(parent),
  _n(at)
{
  if (!pageDummyBrush) {
    pageDummyBrush = new QBrush();

    // Make a texture brush which can be used to print "rendering page" all over
    // the dummy tiles that are shown while the rendering thread is doing its
    // work
    QImage brushTex(1024, 1024, QImage::Format_ARGB32);
    QRectF textRect;
    QPainter p;
    p.begin(&brushTex);
    p.fillRect(brushTex.rect(), Qt::white);
    p.setPen(Qt::lightGray);
    p.drawText(brushTex.rect(), Qt::AlignCenter | Qt::AlignVCenter | Qt::TextSingleLine, QApplication::tr("rendering page"), &textRect);
    p.end();
    textRect.adjust(-textRect.width() * .05, -textRect.height() * .1, textRect.width() * .05, textRect.height() * .1);
    brushTex = brushTex.copy(textRect.toAlignedRect());

    pageDummyBrush->setTextureImage(brushTex);
    pageDummyBrush->setTransform(QTransform().rotate(-45));
  }
}

Page::~Page()
{
}

int Page::pageNum() { return _n; }

QSharedPointer<QImage> Page::getCachedImage(double xres, double yres, QRect render_box)
{
  return _parent->pageCache().getImage(PDFPageTile(xres, yres, render_box, _n));
}

void Page::asyncRenderToImage(QObject *listener, double xres, double yres, QRect render_box, bool cache)
{
  _parent->processingThread().addPageProcessingRequest(new PageProcessingRenderPageRequest(this, listener, xres, yres, render_box, cache));
}

bool higherResolutionThan(const PDFPageTile & t1, const PDFPageTile & t2)
{
  // Note: We silently assume that xres and yres behave the same way
  return t1.xres > t2.xres;
}

QSharedPointer<QImage> Page::getTileImage(QObject * listener, const double xres, const double yres, QRect render_box /* = QRect() */)
{
  // If the render_box is empty, use the whole page
  if (render_box.isNull())
    render_box = QRectF(0, 0, pageSizeF().width() * xres / 72., pageSizeF().height() * yres / 72.).toAlignedRect();

  // If the tile is cached, return it
  QSharedPointer<QImage> retVal = getCachedImage(xres, yres, render_box);
  if (retVal)
    return retVal;

  if (listener) {
    // Render asyncronously, but add a dummy image to the cache first and return
    // that in the end
    // Note: Start the rendering in the background before constructing the image
    // to take advantage of multi-core CPUs. Since we hold the write lock here
    // there's nothing to worry about
    asyncRenderToImage(listener, xres, yres, render_box, true);

    QImage * tmpImg = new QImage(render_box.width(), render_box.height(), QImage::Format_ARGB32);
    QPainter p(tmpImg);
    p.fillRect(tmpImg->rect(), *pageDummyBrush);

    // Look through the cache to find tiles we can reuse (by scaling) for our
    // dummy tile
    // TODO: Benchmark this. If it is actualy too slow (i.e., just keeping the
    // rendered image from popping up due to the write lock we hold) disable it
    {
      QList<PDFPageTile> tiles = _parent->pageCache().tiles();
      for (QList<PDFPageTile>::iterator it = tiles.begin(); it != tiles.end(); ) {
        if (it->page_num != pageNum()) {
          it = tiles.erase(it);
          continue;
        }
        // See if it->render_box intersects with render_box (after proper scaling)
        QRect scaledRect = QTransform::fromScale(xres / it->xres, yres / it->yres).mapRect(it->render_box);
        if (!scaledRect.intersects(render_box)) {
          it = tiles.erase(it);
          continue;
        }
        ++it;
      }
      // Sort the remaining tiles by size, high-res first
      qSort(tiles.begin(), tiles.end(), higherResolutionThan);
      // Finally, crop, scale and paint each image until the whole area is
      // filled or no images are left in the list
      QPainterPath clipPath;
      clipPath.addRect(0, 0, render_box.width(), render_box.height());
      foreach (PDFPageTile tile, tiles) {
        QSharedPointer<QImage> tileImg = _parent->pageCache().getImage(tile);
        if (!tileImg)
          continue;

        // cropRect is the part of `tile` that overlaps the tile-to-paint (after
        // proper scaling).
        // paintRect is the part `tile` fills of the area we paint to (after
        // proper scaling).
        QRect cropRect = QTransform::fromScale(tile.xres / xres, tile.yres / yres).mapRect(render_box).intersected(tile.render_box).translated(-tile.render_box.left(), -tile.render_box.top());
        QRect paintRect = QTransform::fromScale(xres / tile.xres, yres / tile.yres).mapRect(tile.render_box).intersected(render_box).translated(-render_box.left(), -render_box.top());

        // Get the actual image and paint it onto the dummy tile
        QImage tmp(tileImg->copy(cropRect).scaled(paintRect.size()));
        p.setClipPath(clipPath);
        p.drawImage(paintRect.topLeft(), tmp);

        // Confine the clipping path to the part we have not painted to yet.
        QPainterPath pp;
        pp.addRect(paintRect);
        clipPath = clipPath.subtracted(pp);
        if (clipPath.isEmpty())
          break;
      }
    }
    // stop painting or else we couldn't (possibly) delete tmpImg below
    p.end();

    // Add the dummy tile to the cache
    // Note: In the meantime the asynchronous rendering could have finished and
    // insert the final image in the cache---we must handle that case and delete
    // our temporary image
    retVal = _parent->pageCache().setImage(PDFPageTile(xres, yres, render_box, _n), tmpImg, false);
    if (retVal != tmpImg)
      delete tmpImg;
    return retVal;
  }
  else {
    renderToImage(xres, yres, render_box, true);
    return getCachedImage(xres, yres, render_box);
  }
}

void Page::asyncLoadLinks(QObject *listener)
{
  _parent->processingThread().addPageProcessingRequest(new PageProcessingLoadLinksRequest(this, listener));
}

//static
QList<SearchResult> Page::search(SearchRequest request)
{
  if (request.doc.isNull())
    return QList<SearchResult>();
  QSharedPointer<Page> page = request.doc->page(request.pageNum);
  if (page.isNull())
    return QList<SearchResult>();
  return page->search(request.searchString);
}


// vim: set sw=2 ts=2 et

