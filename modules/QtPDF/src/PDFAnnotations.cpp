/**
 * Copyright (C) 2013-2019  Stefan Löffler
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

#include "PDFAnnotations.h"
#include "PDFBackend.h"

namespace QtPDF {

namespace Annotation {

// Annotations
// =================

Markup::~Markup()
{
  delete _popup;
}

Markup::Markup(const Markup & o)
  : AbstractAnnotation(o)
  , _title(o._title)
  , _richContents(o._richContents)
  , _creationDate(o._creationDate)
  , _subject(o._subject)
{
  if (o.popup() != nullptr) {
    setPopup(new Popup(*(o.popup())));
  }
}

Markup & Markup::operator=(const Markup & o)
{
  if (this == &o) {
    return *this;
  }
  AbstractAnnotation::operator=(o);
  _title = o._title;
  _richContents = o._richContents;
  _creationDate = o._creationDate;
  _subject = o._subject;
  if (o.popup() != nullptr) {
    setPopup(new Popup(*(o.popup())));
  }
  else {
    setPopup(nullptr);
  }
  return *this;
}

void Markup::setPopup(Popup * popup)
{
  delete _popup;
  _popup = popup;
  if (_popup != nullptr) {
    _popup->setParent(this);
  }
}


Link::~Link()
{
  delete _actionOnActivation;
}

QPolygonF Link::quadPoints() const
{
  if (_quadPoints.isEmpty())
    return QPolygonF(rect());
  // The PDF specs (1.7) state that: "QuadPoints should be ignored if any
  // coordinate in the array lies outside the region specified by Rect."
  foreach (QPointF p, _quadPoints) {
    if (!rect().contains(p))
      return QPolygonF(rect());
  }
  return _quadPoints;
}

void Link::setActionOnActivation(PDFAction * const action)
{
  delete _actionOnActivation;
  _actionOnActivation = action;
}

} // namespace Annotation

} // namespace QtPDF

// vim: set sw=2 ts=2 et
