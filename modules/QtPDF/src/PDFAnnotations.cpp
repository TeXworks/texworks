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
#include <PDFAnnotations.h>
#include <PDFBackend.h>

// Annotations
// =================

PDFMarkupAnnotation::~PDFMarkupAnnotation()
{
  if (_popup)
    delete _popup;
}

void PDFMarkupAnnotation::setPopup(PDFPopupAnnotation * popup)
{
  if (_popup)
    delete _popup;
  _popup = popup;
}


PDFLinkAnnotation::~PDFLinkAnnotation()
{
  if (_actionOnActivation)
    delete _actionOnActivation;
}

QPolygonF PDFLinkAnnotation::quadPoints() const
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

void PDFLinkAnnotation::setActionOnActivation(PDFAction * const action)
{
  if (_actionOnActivation)
    delete _actionOnActivation;
  _actionOnActivation = action;
}


// vim: set sw=2 ts=2 et

