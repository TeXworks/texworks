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
#include "PDFGuideline.h"

#include "PDFDocumentView.h"

namespace QtPDF {

PDFGuideline::PDFGuideline(PDFDocumentView * parent, const QPoint posWin, const Qt::Orientation orientation)
  : QWidget(parent), m_parent(parent), m_orientation(orientation)
{
  if (parent) {
    m_pageIdx = parent->currentPage();
  }
  switch (m_orientation) {
    case Qt::Horizontal:
      setCursor(Qt::SplitVCursor);
      break;
    case Qt::Vertical:
      setCursor(Qt::SplitHCursor);
      break;
  }
  setOriginWin(posWin);
  connect(m_parent, &PDFDocumentView::updated, this, &PDFGuideline::updatePosition);
  show();
}

void PDFGuideline::updatePosition()
{
  if (!m_parent)
    return;

  setOriginPage(m_originPage);
}

void PDFGuideline::dragMove(const QPoint pos)
{
  setOriginWin(pos);
}

void PDFGuideline::dragStop(const QPoint pos)
{
  if (!m_parent || !m_parent->viewport())
    return;

  const QRect contentsRect = viewContentRect();

  setOriginWin(pos);

  // If the guide line was dragged back to the ruler (or out of the window),
  // delete the guide
  if ((m_orientation == Qt::Horizontal && m_originWin.y() < contentsRect.top()) ||
      (m_orientation == Qt::Vertical && m_originWin.x() < contentsRect.left()))
    deleteLater();
}

void PDFGuideline::setOriginWin(QPoint pt)
{
  m_originWin = pt;
  m_originPage = mapToPage(pt);
  moveAndResize();
}

void PDFGuideline::setOriginPage(QPointF pt)
{
  m_originPage = pt;
  m_originWin = mapFromPage(pt);
  moveAndResize();
}

void PDFGuideline::paintEvent(QPaintEvent * event)
{
  Q_UNUSED(event);

  QPainter painter{this};

  painter.setPen(QPen(Qt::blue, 0));
  QRect contentRect = viewContentRect();

  // NB: Only draw the line if it is over the viewport, not if it over the ruler
  // (hiding the widget when the line is over the ruler is not an option as
  // hiding it while dragging ends the drag, so the line could not be dragged
  // back down)
  switch (m_orientation) {
    case Qt::Horizontal:
      if (m_originWin.y() >= contentRect.top())
        painter.drawLine(0, padding, width(), padding);
      break;
    case Qt::Vertical:
      if (m_originWin.x() >= contentRect.left())
        painter.drawLine(padding, 0, padding, height());
      break;
  }
}

void PDFGuideline::mouseMoveEvent(QMouseEvent *event)
{
  QWidget::mouseMoveEvent(event);
  // NB: as mouse tracking is not enabled for this widget, we only receive mouse
  // move events if a mouse button is held

  if (!event->buttons().testFlag(Qt::LeftButton))
    return;
  if (!m_parent)
    return;

  dragMove(m_parent->mapFromGlobal(event->globalPos()));
  event->accept();
}

void PDFGuideline::mouseReleaseEvent(QMouseEvent *event)
{
  QWidget::mouseReleaseEvent(event);

  if (event->button() != Qt::LeftButton)
    return;

  dragStop(m_parent->mapFromGlobal(event->globalPos()));
  event->accept();
}

void PDFGuideline::moveAndResize()
{
  const QRect contentsRect = viewContentRect();
  QRect newGeometry;
  switch (m_orientation) {
    case Qt::Horizontal:
      newGeometry = QRect(contentsRect.left(), m_originWin.y() - padding, contentsRect.width(), 2 * padding + 1);
      break;
    case Qt::Vertical:
      newGeometry = QRect(m_originWin.x() - padding, contentsRect.top(), 2 * padding + 1, contentsRect.height());
      break;
  }
  if (newGeometry != geometry())
    setGeometry(newGeometry);
}

QPoint PDFGuideline::mapFromPage(const QPointF pt) const
{
  if (!m_parent)
    return {};
  const QWidget * viewport = m_parent->viewport();
  if (!viewport)
    return {};
  const PDFDocumentScene * scene = qobject_cast<PDFDocumentScene*>(m_parent->scene());
  if (!scene)
    return {};
  PDFPageGraphicsItem * page = dynamic_cast<PDFPageGraphicsItem*>(scene->pageAt(m_pageIdx));
  if (!page)
    return {};

  const QPoint ptViewport = m_parent->mapFromScene(page->mapToScene(pt));
  return m_parent->mapFromGlobal(viewport->mapToGlobal(ptViewport));
}

QPointF PDFGuideline::mapToPage(const QPoint pt) const
{
  if (!m_parent)
    return {};
  const QWidget * viewport = m_parent->viewport();
  if (!viewport)
    return {};
  const PDFDocumentScene * scene = qobject_cast<PDFDocumentScene*>(m_parent->scene());
  if (!scene)
    return {};
  PDFPageGraphicsItem * page = dynamic_cast<PDFPageGraphicsItem*>(scene->pageAt(m_pageIdx));
  if (!page)
    return {};

  const QPoint ptViewport = viewport->mapFromGlobal(m_parent->mapToGlobal(pt));
  return page->mapFromScene(m_parent->mapToScene(ptViewport));
}

QRect PDFGuideline::viewContentRect() const
{
  if (!m_parent)
    return {};

  QRect r = m_parent->contentsRect();
  if (m_parent->isRulerVisible())
    return r.marginsRemoved({PDFRuler::rulerSize, PDFRuler::rulerSize, 0, 0});
  else
    return r;
}

} // namespace QtPDF
