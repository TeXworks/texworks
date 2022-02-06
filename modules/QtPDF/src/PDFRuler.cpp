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

#include "PDFRuler.h"

#include "PDFDocumentView.h"

#include <QPainter>

namespace QtPDF {

QString PDFRuler::translatedUnitLabel(const Units &unit)
{
  switch(unit) {
    case CM: return tr("cm");
    case IN: return tr("in");
    case BP: return tr("bp");
  }
  return {};
}

PDFRuler::PDFRuler(PDFDocumentView *parent)
  : QWidget(parent)
{
  m_unitActions.insert(CM, new QAction(translatedUnitLabel(CM), this));
  m_unitActions.insert(IN, new QAction(translatedUnitLabel(IN), this));
  m_unitActions.insert(BP, new QAction(translatedUnitLabel(BP), this));

  Q_FOREACH(const Units & unit, m_unitActions.keys()) {
    QAction * const action = m_unitActions.value(unit);
    action->setCheckable(true);
    action->setChecked(unit == units());
    connect(action, &QAction::triggered, this, [this, unit]() { setUnits(unit); });
    m_contextMenu.addAction(action);
    m_contextMenuActionGroup->addAction(action);
  }
}

void PDFRuler::setUnits(const Units &newUnit)
{
  if (m_Unit == newUnit)
    return;
  m_Unit = newUnit;
  Q_FOREACH(const Units & u, m_unitActions.keys()) {
    QAction * const a = m_unitActions.value(u);
    a->setChecked(u == newUnit);
  }
  update();
}

void PDFRuler::contextMenuEvent(QContextMenuEvent * event)
{
  m_contextMenu.popup(event->globalPos());
}

void PDFRuler::paintEvent(QPaintEvent * event)
{
  Q_UNUSED(event)
  QPainter painter{this};

  // Clear background
  painter.setPen(Qt::black);
  painter.fillRect(rect(), Qt::lightGray);
  painter.drawLine(QPoint(rulerSize, rulerSize), QPointF(width(), rulerSize));
  painter.drawLine(QPoint(rulerSize, rulerSize), QPointF(rulerSize, height()));

  // Obtain pointer to the view and the page object
  PDFDocumentView * docView = qobject_cast<PDFDocumentView*>(parent());
  if (!docView)
    return;
  PDFDocumentScene * scene = qobject_cast<PDFDocumentScene*>(docView->scene());
  if (!scene)
    return;
  PDFPageGraphicsItem * page = dynamic_cast<PDFPageGraphicsItem*>(scene->pageAt(docView->currentPage()));
  if (!page)
    return;

  // Get rect in PDF coordinates (bp) and in pixels
  const QRectF pdfPageRect = page->pointScale().inverted().mapRect(QRectF(QPointF(0, 0), page->pageSizeF()));
  const QRectF pageRect = docView->mapFromScene(page->mapToScene(QRectF(QPoint(0, 0), page->pageSizeF()))).boundingRect().translated(rulerSize, rulerSize);

  // Calculate transforms from px to physical units and back
  const QTransform px2pt = QTransform::fromTranslate(-pageRect.left(), -pageRect.top()) * \
      QTransform::fromScale(pdfPageRect.width() / pageRect.width(), pdfPageRect.height() / pageRect.height()) * \
      QTransform::fromTranslate(pdfPageRect.left(), pdfPageRect.top());

  const QTransform px2phys = [](const QTransform & px2pt, const Units & u) {
    switch (u) {
      case IN: return px2pt * QTransform::fromScale(1. / 72., 1. / 72.);
      case CM: return px2pt * QTransform::fromScale(2.54 / 72., 2.54 / 72.);
      case BP: return px2pt;
    }
    return px2pt;
  }(px2pt, units());
  const QTransform phys2px = px2phys.inverted();

  // Get the viewing rect in physical coordinates
  QRectF physRect = px2phys.mapRect(QRectF(QPointF(rulerSize, rulerSize), rect().bottomRight()));

  QFont font = QGuiApplication::font();
  font.setPixelSize(qFloor(rulerSize * 0.6));
  painter.setFont(font);

  // Draw unit label
  const QString unitLabel = translatedUnitLabel(units());
  painter.drawText(QRectF(0, 0, rulerSize, rulerSize), Qt::AlignCenter, unitLabel);

  const auto calcMajorInterval = [](const qreal physSpacing) {
    const qreal magnitude = qPow(10, qFloor(qLn(physSpacing) / qLn(10)));
    const qreal mantissa = physSpacing / magnitude; // in the interval [1, 10)
    if (mantissa < 2)
      return 1. * magnitude;
    else if (mantissa < 5)
      return 2 * magnitude;
    else
      return 5 * magnitude;
  };

  {
    // Horizontal ruler
    painter.save();
    painter.setClipRect(QRectF(QPointF(rulerSize, 0), QPointF(rect().right(), rulerSize)));
    painter.fillRect(QRectF(QPointF(pageRect.left(), 0), QPointF(pageRect.right(), rulerSize)), Qt::white);
    const qreal dxMajor = calcMajorInterval(px2phys.mapRect(QRectF(QPointF(0, 0), QSizeF(100, 100))).width());
    const int nMinor = 10;
    const qreal dxMinor = dxMajor / nMinor;
    const qreal xMin = qFloor(physRect.left() / dxMajor) * dxMajor;
    for (int i = 0; xMin + i * dxMinor <= physRect.right(); ++i) {
      const Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignTop;
      const qreal xPhys = xMin + i * dxMinor;
      const qreal x = phys2px.map(QPointF(xPhys, 0)).x();
      if (i % nMinor == 0) {
        const QString label = QString::number(xPhys);
        painter.drawLine(QPointF(x, .7 * rulerSize), QPointF(x, rulerSize));
        QRectF boundingRect = painter.boundingRect(QRectF(QPointF(x, 0), QSizeF(0, rulerSize / 2)), alignment, label);
        painter.drawText(boundingRect, alignment, label);
      }
      else {
        painter.drawLine(QPointF(x, .85 * rulerSize), QPointF(x, rulerSize));
      }
    }
    painter.restore();
  }

  {
    // Vertical ruler
    painter.save();
    painter.setClipRect(QRectF(QPointF(0, rulerSize), QPointF(rulerSize, rect().bottom())));
    painter.fillRect(QRectF(QPointF(0, pageRect.top()), QPointF(rulerSize, pageRect.bottom())), Qt::white);
    painter.rotate(-90);
    const qreal dyMajor = calcMajorInterval(px2phys.mapRect(QRectF(QPointF(0, 0), QSizeF(100, 100))).height());
    const int nMinor = 10;
    const qreal dyMinor = dyMajor / nMinor;
    const qreal yMin = qFloor(physRect.top() / dyMajor) * dyMajor;
    for (int i = 0; yMin + i * dyMinor <= physRect.bottom(); ++i) {
      const Qt::Alignment alignment = Qt::AlignHCenter | Qt::AlignTop;
      const qreal yPhys = yMin + i * dyMinor;
      const qreal y = phys2px.map(QPointF(0, yPhys)).y();
      if (y < rulerSize)
        continue;
      if (i % nMinor == 0) {
        const QString label = QString::number(yPhys);
        painter.drawLine(QPointF(-y, .7 * rulerSize), QPointF(-y, rulerSize));
        QRectF boundingRect = painter.boundingRect(QRectF(QPointF(-y, 0), QSizeF(0, rulerSize / 2)), alignment, label);
        painter.drawText(boundingRect, alignment, label);
      }
      else {
        painter.drawLine(QPointF(-y, .85 * rulerSize), QPointF(-y, rulerSize));
      }
    }
    painter.resetTransform();
    painter.restore();
  }
}

void PDFRuler::resizeEvent(QResizeEvent * event)
{
  QRegion mask;
  mask += QRect(QPoint(0, 0), QSize(width(), rulerSize + 1));
  mask += QRect(QPoint(0, 0), QSize(rulerSize + 1, height()));
  setMask(mask);
  QWidget::resizeEvent(event);
}

} // namespace QtPDF
