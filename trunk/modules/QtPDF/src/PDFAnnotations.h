/**
 * Copyright (C) 2011  Stefan Löffler
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
#ifndef PDFAnnotations_H
#define PDFAnnotations_H

#include <PDFActions.h>

#include <QString>
#include <QRectF>
#include <QPolygonF>
#include <QDateTime>
#include <QFlags>
#include <QColor>

namespace QtPDF {

class Page;
class PDFPopupAnnotation;

// ABC for annotations
// Modelled after sec. 8.4.1 of the PDF 1.7 specifications
class PDFAnnotation
{
public:
  enum AnnotationFlag {
    Annotation_Default = 0x0,
    Annotation_Invisible = 0x1,
    Annotation_Hidden = 0x2,
    Annotation_Print = 0x4,
    Annotation_NoZoom = 0x8,
    Annotation_NoRotate = 0x10,
    Annotation_NoView = 0x20,
    Annotation_ReadOnly = 0x40,
    Annotation_Locked = 0x80,
    Annotation_ToggleNoView = 0x100,
    Annotation_LockedContents = 0x200
  };
  Q_DECLARE_FLAGS(AnnotationFlags, AnnotationFlag)

  enum AnnotationType {
    AnnotationTypeText, AnnotationTypeLink, AnnotationTypeFreeText,
    AnnotationTypeLine, AnnotationTypeSquare, AnnotationTypeCircle,
    AnnotationTypePolygon, AnnotationTypePolyLine, AnnotationTypeHighlight,
    AnnotationTypeUnderline, AnnotationTypeSquiggly, AnnotationTypeStrikeOut,
    AnnotationTypeStamp, AnnotationTypeCaret, AnnotationTypeInk,
    AnnotationTypePopup, AnnotationTypeFileAttachment, AnnotationTypeSound,
    AnnotationTypeMovie, AnnotationTypeWidget, AnnotationTypeScreen,
    AnnotationTypePrinterMark, AnnotationTypeTrapNet, AnnotationTypeWatermark,
    AnnotationType3D
  };
  
  PDFAnnotation() : _page(NULL) { }
  virtual ~PDFAnnotation() { }

  virtual AnnotationType type() const = 0;
  virtual bool isMarkup() const { return false; }
  
  // Declare all the getter/setter methods virtual so derived classes can
  // override them
  virtual QRectF rect() const { return _rect; }
  virtual QString contents() const { return _contents; }
  virtual Page * page() const { return _page; }
  virtual QString name() const { return _name; }
  virtual QDateTime lastModified() const { return _lastModified; }
  virtual QFlags<AnnotationFlags> flags() const { return _flags; }
  virtual QFlags<AnnotationFlags>& flags() { return _flags; }
  virtual QColor color() const { return _color; }

  virtual void setRect(const QRectF rect) { _rect = rect; }
  virtual void setContents(const QString contents) { _contents = contents; }
  virtual void setPage(Page * page) { _page = page; }
  virtual void setName(const QString name) { _name = name; }
  virtual void setLastModified(const QDateTime lastModified) { _lastModified = lastModified; }
  virtual void setColor(const QColor color) { _color = color; }

protected:
  QRectF _rect; // required, in pdf coordinates
  QString _contents; // optional
  Page * _page; // optional; since PDF 1.3
  QString _name; // optional; since PDF 1.4
  QDateTime _lastModified; // optional; since PDF 1.1
  // TODO: _appearance, _appearanceState, _border, _structParent, _optContent
  QFlags<AnnotationFlags> _flags;
  // QList<???> _appearance;
  // ??? _appearanceState;
  // ??? _border;
  QColor _color;
  // ??? _structParent;
  // ??? _optContent;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PDFAnnotation::AnnotationFlags)

// Markup Annotation are:
// Text, FreeText, Line, Square, Circle, Polygon, PolyLine, Highlight, Underline
// Squiggly, StrikeOut, Stamp, Caret, Ink, FileAttachment, Sound
class PDFMarkupAnnotation : public PDFAnnotation
{
public:
  PDFMarkupAnnotation() : PDFAnnotation(), _popup(NULL) { }
  virtual ~PDFMarkupAnnotation();

  virtual bool isMarkup() const { return true; }

  virtual QString title() const { return _title; }
  // Synonym for title(), but easier to read
  virtual QString author() const { return _title; }
  virtual QString richContents() const { return (!_richContents.isEmpty() ? _richContents : _contents); }
  virtual QDateTime creationDate() const { return _creationDate; }
  virtual QString subject() const { return _subject; }
  virtual PDFPopupAnnotation * popup() const { return _popup; }

  virtual void setTitle(const QString title) { _title = title; }
  // Synonym for setTitle(), but easier to read
  virtual void setAuthor(const QString author) { _title = author; }
  virtual void setRichContents(const QString contents) { _richContents = contents; }
  virtual void setCreationDate(const QDateTime timestamp) { _creationDate = timestamp; }
  virtual void setSubject(const QString subject) { _subject = subject; }
  // Note: the PDFMarkupAnnotation takes ownership of `popup`
  virtual void setPopup(PDFPopupAnnotation * popup);

protected:
  QString _title; // optional; since PDF 1.1; by convention identifies the annotation author
  PDFPopupAnnotation * _popup;
  // float _opacity;
  QString _richContents; // optional; since PDF 1.5; may contain some HTML tags
  QDateTime _creationDate; // optional; since PDF 1.5
  // PDFAnnotation * _inReplyTo;
  // enum _replyType;
  QString _subject; // optional; since PDF 1.5
  // enum/int _intent;
  // _externalData; // currently only Markup3D
};

class PDFLinkAnnotation : public PDFAnnotation
{
public:
  enum HighlightingMode { HighlightingNone, HighlightingInvert, HighlightingOutline, HighlightingPush };

  PDFLinkAnnotation() : PDFAnnotation(), _actionOnActivation(NULL) { }
  virtual ~PDFLinkAnnotation();
  
  AnnotationType type() const { return AnnotationTypeLink; };

  HighlightingMode highlightingMode() const { return _highlightingMode; }
  QPolygonF quadPoints() const;
  PDFAction * actionOnActivation() const { return _actionOnActivation; }

  void setHighlightingMode(const HighlightingMode mode) { _highlightingMode = mode; }
  void setQuadPoints(const QPolygonF quadPoints) { _quadPoints = quadPoints; }
  // Note: PDFLinkAnnotation takes ownership of PDFAction pointers
  void setActionOnActivation(PDFAction * const action);

private:
  // Note: the PA member of the link annotation dict is deliberately ommitted
  // because we don't support WebCapture at the moment
  // Note: The PDF specs include a "destination" field for LinkAnnotations;
  // In this implementation this case should be handled by a PDFGoToAction
  HighlightingMode _highlightingMode;
  QPolygonF _quadPoints;
  PDFAction * _actionOnActivation;
};

class PDFTextAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeText; }
private:
  bool _open;
  QString _iconName;
  QString _state;
  QString _stateModel;
};

class PDFFreeTextAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeFreeText; }
  // TODO: members
};

class PDFCaretAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeCaret; }
private:
  QRectF _rectDiff;
  // enum _symbol;
};

class PDFPopupAnnotation : public PDFAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypePopup; }
  
  PDFMarkupAnnotation * parent() { return _parent; }
  bool isOpen() const { return _open; }
  
  void setParent(PDFMarkupAnnotation * parent) { _parent = parent; }
  void setOpen(const bool open = true) { _open = open; }
  
private:
  PDFMarkupAnnotation * _parent;
  bool _open;
};

class PDFHighlightAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeHighlight; }
  // TODO: members
};

class PDFUnderlineAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeUnderline; }
  // TODO: members
};

class PDFSquigglyAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeSquiggly; }
  // TODO: members
};

class PDFStrikeOutAnnotation : public PDFMarkupAnnotation
{
public:
  AnnotationType type() const { return AnnotationTypeStrikeOut; }
  // TODO: members
};

// Line, Square, Circle, Polygon, PolyLine, Stamp, Ink, FileAttachment, Sound

} // namespace QtPDF

#endif // End header guard
// vim: set sw=2 ts=2 et

