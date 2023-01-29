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

#ifndef PDFPageCache_H
#define PDFPageCache_H

#include "PDFPageTile.h"

#include <QCache>
#include <QMap>
#include <QReadWriteLock>
#include <QSharedPointer>
#include <QWriteLocker>

class QImage;

namespace QtPDF {

namespace Backend {

// This class is thread-safe
class PDFPageCache : protected QCache<PDFPageTile, QSharedPointer<QImage> >
{
  typedef QCache<PDFPageTile, QSharedPointer<QImage> > Super;

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
  using size_type = int;
#else
  using size_type = qsizetype;
#endif
public:
  enum TileStatus { UNKNOWN, PLACEHOLDER, CURRENT, OUTDATED };

  PDFPageCache() = default;
  virtual ~PDFPageCache() = default;

  // Note: Each image has a cost of 1
  size_type maxSize() const { return maxCost(); }
  void setMaxSize(const size_type num) { setMaxCost(num); }

  // Returns the image under the key `tile` or nullptr if it doesn't exist
  QSharedPointer<QImage> getImage(const PDFPageTile & tile) const;
  TileStatus getStatus(const PDFPageTile & tile) const;
  // Returns the pointer to the image in the cache under the key `tile` after
  // the insertion. If overwrite == true, this will always be image, otherwise
  // it can be different
  QSharedPointer<QImage> setImage(const PDFPageTile & tile, QImage * image, const TileStatus status, const bool overwrite = true);

  void lock() const { _lock.lockForRead(); }
  void unlock() const { _lock.unlock(); }

  void clear() { QWriteLocker l(&_lock); Super::clear(); _tileStatus.clear(); }
  // Mark all tiles outdated
  void markOutdated();

  QList<PDFPageTile> tiles() const { return keys(); }
protected:
  mutable QReadWriteLock _lock;
  // Map to keep track of the current status of tiles; note that the status
  // information is not deleted when the QCache scraps images to save memory.
  QMap<PDFPageTile, TileStatus> _tileStatus;
};

} // namespace Backend

} // namespace QtPDF

#endif // !defined(PDFPageCache_H)
