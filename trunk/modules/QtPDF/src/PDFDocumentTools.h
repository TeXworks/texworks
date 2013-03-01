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
#ifndef PDFDocumentTools_H
#define PDFDocumentTools_H

#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QCursor>
#include <QRubberBand>
#include <QComboBox>
#include <QGraphicsView>
#include <QGraphicsLineItem>
#include <QGraphicsProxyWidget>

#ifdef DEBUG
  #include <QDebug>
#endif

namespace QtPDF {

class PDFDocumentView;
class PDFDocumentMagnifierView;

namespace DocumentTool {

class AbstractTool
{
  friend class QtPDF::PDFDocumentView;
public:
  enum Type { Tool_None, Tool_MagnifyingGlass, Tool_ZoomIn, Tool_ZoomOut, Tool_MarqueeZoom, Tool_Move, Tool_ContextMenu, Tool_ContextClick, Tool_Measure };
  AbstractTool(PDFDocumentView * parent) : _parent(parent), _cursor(QCursor(Qt::ArrowCursor)) { }
  virtual ~AbstractTool() { }
  
  virtual Type type() const { return Tool_None; }
  virtual bool operator==(const AbstractTool & o) { return (type() == o.type()); }
protected:
  virtual void arm();
  virtual void disarm();

  // By default, key events will call the parent view's maybeArmTool(). Derived
  // classes that rely on key events can override this behavior to handle
  // certain key events itself.
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event) { }
  virtual void mouseReleaseEvent(QMouseEvent * event);
  virtual void paintEvent(QPaintEvent * event) { }

  PDFDocumentView * _parent;
  QCursor _cursor;
};

class ZoomIn : public AbstractTool
{
public:
  ZoomIn(PDFDocumentView * parent);
  virtual Type type() const { return Tool_ZoomIn; }
protected:
  virtual void arm() { AbstractTool::arm(); _started = false; }
  virtual void disarm() { AbstractTool::disarm(); _started = false; }

  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  QPoint _startPos;
  bool _started;
};

class ZoomOut : public AbstractTool
{
public:
  ZoomOut(PDFDocumentView * parent);
  virtual Type type() const { return Tool_ZoomOut; }
protected:
  virtual void arm() { AbstractTool::arm(); _started = false; }
  virtual void disarm() { AbstractTool::disarm(); _started = false; }

  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  QPoint _startPos;
  bool _started;
};

class MagnifyingGlass : public AbstractTool
{
public:
  enum MagnifierShape { Magnifier_Rectangle, Magnifier_Circle };
  MagnifyingGlass(PDFDocumentView * parent);
  virtual Type type() const { return Tool_MagnifyingGlass; }
  PDFDocumentMagnifierView * magnifier() { return _magnifier; }

  void setMagnifierShape(const MagnifierShape shape);
  void setMagnifierSize(const int size);

protected:
  virtual void arm() { AbstractTool::arm(); _started = false; }
  virtual void disarm() { AbstractTool::disarm(); _started = false; }

  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  virtual void paintEvent(QPaintEvent * event);

  PDFDocumentMagnifierView * _magnifier;
  bool _started;
};

class MarqueeZoom : public AbstractTool
{
public:
  MarqueeZoom(PDFDocumentView * parent);
  virtual Type type() const { return Tool_MarqueeZoom; }
protected:
  virtual void arm() { AbstractTool::arm(); _started = false; }
  virtual void disarm() { AbstractTool::disarm(); _started = false; }

  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);

  bool _started;
  QPoint _startPos;
  QRubberBand * _rubberBand;
};

class Move : public AbstractTool
{
public:
  Move(PDFDocumentView * parent);
  virtual Type type() const { return Tool_Move; }
protected:
  virtual void arm() { AbstractTool::arm(); _started = false; }
  virtual void disarm() { AbstractTool::disarm(); _started = false; }

  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);

  bool _started;
  QPoint _oldPos;
  QCursor _closedHandCursor;
};

class ContextClick : public AbstractTool
{
public:
  ContextClick(PDFDocumentView * parent) : AbstractTool(parent) { }
  virtual Type type() const { return Tool_ContextClick; }
protected:
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  bool _started;
};

class Measure;
class MeasureLine;

class MeasureLineGrip : public QGraphicsRectItem
{
  friend class Measure;
public:
  // pt should be 1 for the first handle, 2 for the second handle
  MeasureLineGrip(MeasureLine * parent, const int pt);
protected:
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  
  // to be called from friend class Measure during creation of the MeasureLine
  void mouseMove(const QPointF scenePos, const Qt::KeyboardModifiers modifiers);
  
  // 1 for the first handle, 2 for the second handle
  int _pt;
};

class MeasureLine : public QGraphicsLineItem
{
  friend class Measure;
public:
  MeasureLine(QGraphicsView * primaryView, QGraphicsItem * parent = NULL);
  virtual ~MeasureLine() { }

  void setLine(qreal x1, qreal y1, qreal x2, qreal y2) { setLine(QLineF(x1, y1, x2, y2)); }
  void setLine(QPointF p1, QPointF p2) { setLine(QLineF(p1, p2)); }
  void setLine(QLineF line);
  virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
protected:
  void updateMeasurement();
  void updateMeasureBoxPos();
  
  virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
  virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);
  virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
  virtual void mouseMoveEvent(QGraphicsSceneMouseEvent *event);
  
  QComboBox * _measureBox;
  QGraphicsProxyWidget * _measureBoxProxy;
  MeasureLineGrip * _grip1, * _grip2;
  QMap<QString, float> _measures;
  QGraphicsView * _primaryView;
  QPointF _grabOffset;
};

class Measure : public AbstractTool
{
public:
  Measure(PDFDocumentView * parent);
  virtual Type type() const { return Tool_Measure; }
protected:
  virtual void mousePressEvent(QMouseEvent * event);
  virtual void mouseMoveEvent(QMouseEvent *event);
  virtual void mouseReleaseEvent(QMouseEvent * event);
  virtual void keyPressEvent(QKeyEvent *event);
  virtual void keyReleaseEvent(QKeyEvent *event);
  
  MeasureLine * _measureLine;
  bool _started;
  QPoint _startPos;
};

} // namepsace DocumentTool

} // namespace QtPDF

#endif // End header guard
// vim: set sw=2 ts=2 et

