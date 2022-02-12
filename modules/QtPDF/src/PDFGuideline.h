/**
 * Copyright (C) 2022  Stefan Löffler
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
#ifndef PDFGUIDELINE_H
#define PDFGUIDELINE_H

#include <QWidget>

namespace QtPDF {

class PDFDocumentView;
class PDFPageGraphicsItem;

class PDFGuideline : public QWidget
{
  Q_OBJECT
  constexpr static int padding = 2;

  PDFDocumentView * m_parent{nullptr};
  int m_pageIdx{-1};
  QPointF m_originPage;
  QPoint m_originWin;
  Qt::Orientation m_orientation{Qt::Horizontal};

public:
  PDFGuideline(PDFDocumentView * parent, const QPoint posWin, const Qt::Orientation orientation);

  void updatePosition();

  void dragMove(const QPoint pos);
  void dragStop(const QPoint pos);

  QPoint originWin() const { return m_originWin; }
  QPointF originPage() const { return m_originPage; }
  void setOriginWin(QPoint pt);
  void setOriginPage(QPointF pt);

protected:
  void paintEvent(QPaintEvent * event) override;
  void mouseMoveEvent(QMouseEvent * event) override;
  void mouseReleaseEvent(QMouseEvent * event) override;

  void moveAndResize();

  QPoint mapFromPage(const QPointF pt) const;
  QPointF mapToPage(const QPoint pt) const;
  QRect viewContentRect() const;
};

} // namespace QtPDF

#endif // PDFGUIDELINE_H
