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

// MeasureLineGrip
// ===========================
//
MeasureLineGrip::MeasureLineGrip(MeasureLine * parent, const int pt) :
  QGraphicsRectItem(parent),
  _pt(pt)
{
  setFlag(QGraphicsItem::ItemIgnoresTransformations);
  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);
  setRect(-2, -2, 5, 5);
  setBrush(QBrush(Qt::green));
  setPen(QPen(Qt::black));
}

void MeasureLineGrip::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  setCursor(Qt::CrossCursor);
  QGraphicsRectItem::hoverEnterEvent(event);
}

void MeasureLineGrip::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  unsetCursor();
  QGraphicsRectItem::hoverLeaveEvent(event);
}

void MeasureLineGrip::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // mousePressEvent must be implemented to receive mouseMoveEvent messages
}

void MeasureLineGrip::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  Q_ASSERT(event != NULL);
  mouseMove(event->scenePos(), event->modifiers());
}

void MeasureLineGrip::mouseMove(const QPointF scenePos, const Qt::KeyboardModifiers modifiers)
{
  MeasureLine * ml = static_cast<MeasureLine*>(parentItem());
  Q_ASSERT(ml != NULL);
  
  switch(_pt) {
  case 1:
  {
    QLineF line(scenePos, ml->line().p2());
    if (modifiers.testFlag(Qt::ControlModifier)) {
      // locked mode; horizontal or vertical line only
      if (line.angle() <= 45 || line.angle() >= 315 ||
          (line.angle() >= 135 && line.angle() <= 225))
        line.setP1(QPointF(line.p1().x(), line.p2().y()));
      else
        line.setP1(QPointF(line.p2().x(), line.p1().y()));
    }
    ml->setLine(line);
    break;
  }
  default:
  {
    QLineF line(ml->line().p1(), scenePos);
    if (modifiers.testFlag(Qt::ControlModifier)) {
      // locked mode; horizontal or vertical line only
      if (line.angle() <= 45 || line.angle() >= 315 ||
          (line.angle() >= 135 && line.angle() <= 225))
        line.setP2(QPointF(line.p2().x(), line.p1().y()));
      else
        line.setP2(QPointF(line.p1().x(), line.p2().y()));
    }
    ml->setLine(line);
    break;
  }
  }
}

// MeasureLine
// ===========================
//
MeasureLine::MeasureLine(QGraphicsView * primaryView, QGraphicsItem * parent /* = NULL */) :
  QGraphicsLineItem(parent),
  _primaryView(primaryView)
{
  _measureBox = new QComboBox();
  _measureBox->setSizeAdjustPolicy(QComboBox::AdjustToContents);
  _measureBoxProxy = new QGraphicsProxyWidget(this);
  _measureBoxProxy->setWidget(_measureBox);
  _measureBoxProxy->setFlag(QGraphicsItem::ItemIgnoresTransformations);
  _measureBoxProxy->setFlag(QGraphicsItem::ItemSendsGeometryChanges);

  setAcceptHoverEvents(true);
  setAcceptedMouseButtons(Qt::LeftButton);

  _grip1 = new MeasureLineGrip(this, 1);
  _grip2 = new MeasureLineGrip(this, 2);
}

void MeasureLine::setLine(QLineF line)
{
  QGraphicsLineItem::setLine(line);
  
  _grip1->setPos(line.p1());
  _grip2->setPos(line.p2());
  updateMeasurement();
}

void MeasureLine::updateMeasurement()
{
  Q_ASSERT(_measureBox != NULL);
  
  // Length of the measurement line in pt (i.e., 1/72.27 inch)
  // Note: we use LaTeX units here, i.e., 1 pt = 1/72.27 in (as opposed to the
  // pdf unit 1 pt = 1/72 in, which in this context is called 1 bp); see
  // http://en.wikibooks.org/wiki/LaTeX/Useful_Measurement_Macros
  float length = line().length() / 1.00375;
  
  int idx = _measureBox->currentIndex();
  _measureBox->clear();
  _measureBox->addItem(QString::fromUtf8("%1 pt").arg(length), QString::fromUtf8("pt"));
  _measureBox->addItem(QString::fromUtf8("%1 mm").arg(length / 2.84), QString::fromUtf8("mm"));
  _measureBox->addItem(QString::fromUtf8("%1 cm").arg(length / 28.4), QString::fromUtf8("cm"));
  _measureBox->addItem(QString::fromUtf8("%1 in").arg(length / 72.27), QString::fromUtf8("in"));
  _measureBox->addItem(QString::fromUtf8("%1 bp").arg(length * 1.00375), QString::fromUtf8("bp"));
  _measureBox->addItem(QString::fromUtf8("%1 pc").arg(length / 12), QString::fromUtf8("pc"));
  _measureBox->addItem(QString::fromUtf8("%1 dd").arg(length / 1.07), QString::fromUtf8("dd"));
  _measureBox->addItem(QString::fromUtf8("%1 cc").arg(length / 12.84), QString::fromUtf8("cc"));
  _measureBox->addItem(QString::fromUtf8("%1 sp").arg(length * 65536), QString::fromUtf8("sp"));
  if (idx < 0 || idx >= _measureBox->count())
    idx = 0;
  _measureBox->setCurrentIndex(idx);
  _measureBox->updateGeometry();
  // Since the box size may have changed, we need to reposition the box
  updateMeasureBoxPos();
}

void MeasureLine::updateMeasureBoxPos()
{
  const float ALMOST_ZERO = 1e-4;
  Q_ASSERT(_primaryView != NULL);
  Q_ASSERT(_measureBoxProxy != NULL);
  Q_ASSERT(_measureBox != NULL);
  
  QPointF center = line().pointAt(0.5);
  // scaling of a unit square
  float scaling = _primaryView->mapToScene(0, 0, 1, 1).boundingRect().width();
  // spacing of 2 pixels (mapped to scene coordinates)
  float spacing = 2 * scaling;
  QPointF offset;
  
  // Get the size of the measurement box in scene coordinates
  QSizeF sceneSize = scaling * _measureBox->size();

  // horizontal line
  if ((line().angle() <= ALMOST_ZERO || line().angle() > 360 - ALMOST_ZERO) ||
      (line().angle() >= 180 - ALMOST_ZERO && line().angle() < 180 + ALMOST_ZERO))
    offset = QPointF(-sceneSize.width() / 2., spacing);
  // vertical line
  else if ((line().angle() >= 90 - ALMOST_ZERO && line().angle() < 90 + ALMOST_ZERO) ||
           (line().angle() >= 270 - ALMOST_ZERO && line().angle() < 270 + ALMOST_ZERO))
    offset = QPointF(spacing, -sceneSize.height() / 2);
  // line pointing up
  else if (line().angle() < 90 || (line().angle() > 180 && line().angle() < 270))
    offset = QPointF(spacing / 1.41421356237, spacing / 1.41421356237);
  // line pointing down
  else
    offset = QPointF(spacing / 1.41421356237, -sceneSize.height() - spacing / 1.41421356237);
  
  _measureBoxProxy->setPos(center + offset);
}

void MeasureLine::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
  QGraphicsLineItem::paint(painter, option, widget);
  // TODO: Possibly change style of pen
  
  // TODO: Only reposition measurement box if zoom level changed
  updateMeasureBoxPos();
}

void MeasureLine::hoverEnterEvent(QGraphicsSceneHoverEvent *event)
{
  setCursor(Qt::SizeAllCursor);
}

void MeasureLine::hoverLeaveEvent(QGraphicsSceneHoverEvent *event)
{
  unsetCursor();
}

void MeasureLine::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
  // Find the offset of the grab point from handle 1
  _grabOffset = event->scenePos() - line().p1();
}

void MeasureLine::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
  // Note: We only receive this event while dragging (i.e., after a
  // mousePressEvent)
  setLine(QLineF(event->scenePos() - _grabOffset, event->scenePos() - _grabOffset + line().p2() - line().p1()));
}


// Measure
// ===========================
//
Measure::Measure(PDFDocumentView * parent) :
  AbstractTool(parent),
  _measureLine(NULL),
  _started(false)
{
}

void Measure::mousePressEvent(QMouseEvent * event)
{
  Q_ASSERT(_parent != NULL);
  if (event && _parent->scene()) {
    _started = (event->buttons() == Qt::LeftButton && event->button() == Qt::LeftButton);
    if (!_started)
      return;
    if (!_measureLine) {
      _measureLine = new MeasureLine(_parent);
      _parent->scene()->addItem(_measureLine);
    }
    _measureLine->setLine(_parent->mapToScene(event->pos()), _parent->mapToScene(event->pos()));
    // Initialize with a hidden measuring line with length 0. This way, simple
    // clicking can be used to remove (i.e., hide) a previously visible line
    _measureLine->hide();
    _started = true;
    _startPos = event->pos();
  }
}

void Measure::mouseMoveEvent(QMouseEvent *event)
{
  if (_started) {
    Q_ASSERT(_measureLine != NULL);
    Q_ASSERT(_measureLine->_grip2 != NULL);
    _measureLine->_grip2->mouseMove(_parent->mapToScene(event->pos()), event->modifiers());
    if ((event->pos() - _startPos).manhattanLength() > QApplication::startDragDistance() && !_measureLine->isVisible())
      _measureLine->show();
  }
}

void Measure::mouseReleaseEvent(QMouseEvent * event)
{
  _started = false;
}

void Measure::keyPressEvent(QKeyEvent *event)
{
  // We need to hijack the Ctrl key modifier here (if the tool is started;
  // otherwise we pass it on (e.g., to a DocumentTool::ContextClick))
  if (event->key() != Qt::Key_Control || !_started)
    AbstractTool::keyPressEvent(event);
}

void Measure::keyReleaseEvent(QKeyEvent *event)
{
  // We need to hijack the Ctrl key modifier here (if the tool is started;
  // otherwise we pass it on (e.g., to a DocumentTool::ContextClick))
  if (event->key() != Qt::Key_Control || !_started)
    AbstractTool::keyReleaseEvent(event);
}


} // namespace DocumentTool
} // namespace QtPDF

// vim: set sw=2 ts=2 et

