/**
 * Copyright (C) 2013-2023  Stefan Löffler, Charlie Sharpsteen
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

#include "PDFPageCache.h"

#include <QImage>

namespace QtPDF {

namespace Backend {

QSharedPointer<QImage> PDFPageCache::getImage(const PDFPageTile & tile) const
{
  QReadLocker locker(&_lock);
  QSharedPointer<QImage> * retVal = m_cache.object(tile);
  if (retVal)
    return *retVal;
  return QSharedPointer<QImage>();
}

PDFPageCache::TileStatus PDFPageCache::getStatus(const PDFPageTile & tile) const
{
  PDFPageCache::TileStatus retVal = UNKNOWN;
  QReadLocker locker(&_lock);
  if (_tileStatus.contains(tile))
    retVal = _tileStatus[tile];
  return retVal;
}

QSharedPointer<QImage> PDFPageCache::setImage(const PDFPageTile & tile, QImage * image, const TileStatus status, const bool overwrite /* = true */)
{
  QWriteLocker locker(&_lock);
  QSharedPointer<QImage> retVal;
  if (m_cache.contains(tile))
    retVal = *(m_cache.object(tile));
  // If the key is not in the cache yet add it. Otherwise overwrite the cached
  // image but leave the pointer intact as that can be held/used elsewhere
  if (!retVal) {
    QSharedPointer<QImage> * toInsert = new QSharedPointer<QImage>(image);
#if QT_VERSION < QT_VERSION_CHECK(5, 10, 0)
    m_cache.insert(tile, toInsert, (image ? image->byteCount() : 0));
#else
    // No image (1024x124x4 bytes by default) should ever come even close to the
    // 2 GB mark corresponding to INT_MAX; note that Document::Document() sets
    // the cache's max-size to 1 GB total
    m_cache.insert(tile, toInsert, (image ? static_cast<int>(image->sizeInBytes()) : 0));
#endif
    _tileStatus.insert(tile, status);
    retVal = *toInsert;
  }
  else if (retVal.data() == image) {
    // Trying to overwrite an image with itself - just update the status
    _tileStatus.insert(tile, status);
  }
  else if (overwrite) {
    // TODO: overwriting an image with a different one can change its size (and
    // therefore its cost in the cache). There doesn't seem to be a method to
    // hande that in QCache, though, and since we only use one tile size this
    // shouldn't pose a problem.
    if (image)
      *retVal = *image;
    else {
      QSharedPointer<QImage> * toInsert = new QSharedPointer<QImage>;
      m_cache.insert(tile, toInsert, 0);
      retVal = *toInsert;
    }
    _tileStatus.insert(tile, status);
  }
  return retVal;
}

void PDFPageCache::markOutdated()
{
  QWriteLocker l(&_lock);
  QMap<PDFPageTile, TileStatus>::iterator it;
  for (it = _tileStatus.begin(); it != _tileStatus.end(); ++it)
    it.value() = OUTDATED;
}

} // namespace Backend

} // namespace QtPDF
