/**
 * Copyright (C) 2012  Stefan Löffler
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
#include <PDFDocumentTools.h>
#include <PDFDocumentView.h>

namespace QtPDF {
namespace DocumentTool {

// AbstractTool
// ========================
//
void AbstractTool::arm() {
  Q_ASSERT(_parent != NULL);
  if (_parent->viewport())
    _parent->viewport()->setCursor(_cursor);
}
void AbstractTool::disarm() {
  Q_ASSERT(_parent != NULL);
  if (_parent->viewport())
    _parent->viewport()->unsetCursor();
}

void AbstractTool::keyPressEvent(QKeyEvent *event)
{
  if (_parent)
    _parent->maybeArmTool(Qt::LeftButton + event->modifiers());
}

void AbstractTool::keyReleaseEvent(QKeyEvent *event)
{
  if (_parent)
    _parent->maybeArmTool(Qt::LeftButton + event->modifiers());
}

void AbstractTool::mousePressEvent(QMouseEvent * event)
{
  if (_parent)
    _parent->maybeArmTool(event->buttons() | event->modifiers());
}

void AbstractTool::mouseReleaseEvent(QMouseEvent * event)
{
  // If the last mouse button was released, we arm the tool corresponding to the
  // left mouse button by default
  Qt::MouseButtons buttons = event->buttons();
  if (buttons == Qt::NoButton)
    buttons |= Qt::LeftButton;

  if (_parent)
    _parent->maybeArmTool(buttons | event->modifiers());
}


// ZoomIn
// ========================
//
ZoomIn::ZoomIn(PDFDocumentView * parent)
: AbstractTool(parent),
  _started(false)
{
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/zoomincursor.png")));
}

void ZoomIn::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started)
    _startPos = event->pos();
}

void ZoomIn::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPoint offset = event->pos() - _startPos;
    if (offset.manhattanLength() <  QApplication::startDragDistance())
      _parent->zoomIn();
  }
  _started = false;
}

// ZoomOut
// ========================
//
ZoomOut::ZoomOut(PDFDocumentView * parent)
: AbstractTool(parent),
  _started(false)
{
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/zoomoutcursor.png")));
}

void ZoomOut::mousePressEvent(QMouseEvent * event)
{
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started)
    _startPos = event->pos();
}

void ZoomOut::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPoint offset = event->pos() - _startPos;
    if (offset.manhattanLength() <  QApplication::startDragDistance())
      _parent->zoomOut();
  }
  _started = false;
}


// MagnifyingGlass
// ==============================
//
MagnifyingGlass::MagnifyingGlass(PDFDocumentView * parent) : 
  AbstractTool(parent)
{
  _magnifier = new PDFDocumentMagnifierView(parent);
  _cursor = QCursor(QPixmap(QString::fromUtf8(":/icons/magnifiercursor.png")));
}

void MagnifyingGlass::setMagnifierShape(const MagnifierShape shape)
{
  Q_ASSERT(_magnifier != NULL);
  _magnifier->setShape(shape);
}

void MagnifyingGlass::setMagnifierSize(const int size)
{
  Q_ASSERT(_magnifier != NULL);
  _magnifier->setSize(size);
}

void MagnifyingGlass::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);

  if (_started) {
    _magnifier->prepareToShow();
    _magnifier->setPosition(event->pos());
  }
  _magnifier->setVisible(_started);

  // Ensure an update of the viewport so that the drop shadow is painted
  // correctly
  QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
  r.moveCenter(_magnifier->geometry().center());
  _parent->viewport()->update(r);
}

void MagnifyingGlass::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;

  // Only update the portion of the viewport (possibly) obscured by the
  // magnifying glass and its shadow.
  QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
  r.moveCenter(_magnifier->geometry().center());
  _parent->viewport()->update(r);

  _magnifier->setPosition(event->pos());
}

void MagnifyingGlass::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_magnifier != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    _magnifier->hide();
    _started = false;
    // Force an update of the viewport so that the drop shadow is hidden
    QRect r(QPoint(0, 0), _magnifier->dropShadow().size());
    r.moveCenter(_magnifier->geometry().center());
    _parent->viewport()->update(r);
  }
}

void MagnifyingGlass::paintEvent(QPaintEvent * event)
{
  Q_ASSERT(_magnifier != NULL);
  Q_ASSERT(_parent != NULL);

  if (!_started)
    return;

  // Draw a drop shadow
  QPainter p(_parent->viewport());
  QPixmap& dropShadow(_magnifier->dropShadow());
  QRect r(QPoint(0, 0), dropShadow.size());
  r.moveCenter(_magnifier->geometry().center());
  p.drawPixmap(r.topLeft(), dropShadow);
}


// MarqueeZoom
// ==========================
//
MarqueeZoom::MarqueeZoom(PDFDocumentView * parent) :
  AbstractTool(parent)
{
  Q_ASSERT(_parent);
  _rubberBand = new QRubberBand(QRubberBand::Rectangle, _parent->viewport());
  _cursor = QCursor(Qt::CrossCursor);
}

void MarqueeZoom::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  Q_ASSERT(_rubberBand != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started) {
    _startPos = event->pos();
    _rubberBand->setGeometry(QRect());
    _rubberBand->show();
  }
}

void MarqueeZoom::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_rubberBand != NULL);

  QPoint o = _startPos, p = event->pos();

  if (event->buttons() != Qt::LeftButton ) {
    // The user somehow let go of the left button without us recieving an
    // event. Abort the zoom operation.
    _rubberBand->setGeometry(QRect());
    _rubberBand->hide();
  } else if ( (o - p).manhattanLength() > QApplication::startDragDistance() ) {
    // Update rubber band Geometry.
    _rubberBand->setGeometry(QRect(
      QPoint(qMin(o.x(),p.x()), qMin(o.y(), p.y())),
      QPoint(qMax(o.x(),p.x()), qMax(o.y(), p.y()))
    ));
  }
}

void MarqueeZoom::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QRectF zoomRect = _parent->mapToScene(_rubberBand->geometry()).boundingRect();
    _rubberBand->hide();
    _rubberBand->setGeometry(QRect());
    _parent->zoomToRect(zoomRect);
  }
  _started = false;
}


// Move
// ===================
//
Move::Move(PDFDocumentView * parent) :
  AbstractTool(parent)
{
  _cursor = QCursor(Qt::OpenHandCursor);
  _closedHandCursor = QCursor(Qt::ClosedHandCursor);
}

void Move::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
  if (_started) {
    if (_parent->viewport())
      _parent->viewport()->setCursor(_closedHandCursor);
    _oldPos = event->pos();
  }
}

void Move::mouseMoveEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!_started || !event)
    return;

  // Adapted from <qt>/src/gui/graphicsview/qgraphicsview.cpp @ QGraphicsView::mouseMoveEvent
  QScrollBar *hBar = _parent->horizontalScrollBar();
  QScrollBar *vBar = _parent->verticalScrollBar();
  QPoint delta = event->pos() - _oldPos;
  hBar->setValue(hBar->value() - delta.x());
  vBar->setValue(vBar->value() - delta.y());
  _oldPos = event->pos();
}

void Move::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton)
    if (_parent->viewport())
      _parent->viewport()->setCursor(_cursor);
  _started = false;
}


// ContextClick
// ===========================
//
void ContextClick::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  
  if (!event)
    return;
  _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
}

void ContextClick::mouseReleaseEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);

  if (!event || !_started)
    return;

  _started = false;
  if (event->buttons() == Qt::NoButton && event->button() == Qt::LeftButton) {
    QPointF pos(_parent->mapToScene(event->pos()));
    QGraphicsItem * item = _parent->scene()->itemAt(pos);
    if (!item || item->type() != PDFPageGraphicsItem::Type)
      return;
    PDFPageGraphicsItem * pageItem = static_cast<PDFPageGraphicsItem*>(item);
    _parent->triggerContextClick(pageItem->page()->pageNum(), pageItem->mapToPage(pageItem->mapFromScene(pos)));
  }
}

} // namespace DocumentTool
} // namespace QtPDF

// vim: set sw=2 ts=2 et

