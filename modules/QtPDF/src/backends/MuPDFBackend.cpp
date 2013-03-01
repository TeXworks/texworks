/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3, or (at your option) any later
 * version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 */

// NOTE: `MuPDFBackend.h` is included via `PDFBackend.h`
#include <PDFBackend.h>

QRectF toRectF(const fz_rect r)
{
  return QRectF(QPointF(r.x0, r.y0), QPointF(r.x1, r.y1));
}

// TODO: Find a better place to put this
PDFDestination toPDFDestination(pdf_xref * xref, fz_obj * dest)
{
  PDFDestination retVal;

  if (!dest)
    return PDFDestination();
  if (fz_is_name(dest))
    return PDFDestination(QString::fromAscii(fz_to_name(dest)));
  if (fz_is_string(dest))
    return PDFDestination(QString::fromAscii(fz_to_str_buf(dest)));

  if (fz_is_array(dest)) {
    // Structure of pdf_link->dest:
    // array w/ >= 2 entries:
    //  - dict which can be passed to pdf_find_page_number
    //  - name: /XYZ, /FIT, ...
    //  - numbers or nulls as required by /XYZ, /FIT, ...
    // TODO: How do external links look like? Or are remote gotos not
    // included in pdf_link structures? Do we need to go through all
    // annotations, then?
    // TODO: Check other types than /XYZ
    if (fz_array_len(dest) < 2)
      return PDFDestination();

    fz_obj * obj;

    obj = fz_array_get(dest, 0);
    if (fz_is_int(obj))
      retVal.setPage(fz_to_int(obj));
    else if (fz_is_dict(obj))
      retVal.setPage(pdf_find_page_number(xref, obj));
    else
      return PDFDestination();

    if (!fz_is_name(fz_array_get(dest, 1)))
      return PDFDestination();
    QString type = QString::fromAscii(fz_to_name(fz_array_get(dest, 1)));
    float left, top, bottom, right, zoom;

    // /XYZ left top zoom 
    if (type == QString::fromUtf8("XYZ")) {
      if (fz_array_len(dest) != 5)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        left = fz_to_real(obj);
      else if (fz_is_null(obj))
        left = -1;
      else
        return PDFDestination();
      obj = fz_array_get(dest, 3);
      if (fz_is_real(obj) || fz_is_int(obj))
        top = fz_to_real(obj);
      else if (fz_is_null(obj))
        top = -1;
      else
        return PDFDestination();
      obj = fz_array_get(dest, 4);
      if (fz_is_real(obj) || fz_is_int(obj))
        zoom = fz_to_real(obj);
      else if (fz_is_null(obj))
        zoom = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_XYZ);
      retVal.setRect(QRectF(QPointF(left, top), QPointF(-1, -1)));
      retVal.setZoom(zoom);
    }
    // /Fit
    else if (type == QString::fromUtf8("Fit")) {
      if (fz_array_len(dest) != 2)
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_Fit);
    }
    // /FitH top
    else if (type == QString::fromUtf8("FitH")) {
      if (fz_array_len(dest) != 3)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        top = fz_to_real(obj);
      else if (fz_is_null(obj))
        top = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitH);
      retVal.setRect(QRectF(QPointF(-1, top), QPointF(-1, -1)));
    }
    // /FitV left
    else if (type == QString::fromUtf8("FitV")) {
      if (fz_array_len(dest) != 3)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        left = fz_to_real(obj);
      else if (fz_is_null(obj))
        left = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitV);
      retVal.setRect(QRectF(QPointF(left, -1), QPointF(-1, -1)));
    }
    // /FitR left bottom right top
    if (type == QString::fromUtf8("FitR")) {
      if (fz_array_len(dest) != 6)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        left = fz_to_real(obj);
      else if (fz_is_null(obj))
        left = -1;
      else
        return PDFDestination();
      obj = fz_array_get(dest, 3);
      if (fz_is_real(obj) || fz_is_int(obj))
        bottom = fz_to_real(obj);
      else if (fz_is_null(obj))
        bottom = -1;
      else
        return PDFDestination();
      obj = fz_array_get(dest, 4);
      if (fz_is_real(obj) || fz_is_int(obj))
        right = fz_to_real(obj);
      else if (fz_is_null(obj))
        right = -1;
      else
        return PDFDestination();
      obj = fz_array_get(dest, 5);
      if (fz_is_real(obj) || fz_is_int(obj))
        top = fz_to_real(obj);
      else if (fz_is_null(obj))
        top = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitR);
      retVal.setRect(QRectF(QPointF(left, top), QPointF(right, bottom)));
      retVal.setZoom(zoom);
    }
    // /FitB
    else if (type == QString::fromUtf8("FitB")) {
      if (fz_array_len(dest) != 2)
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitB);
    }
    // /FitBH top
    else if (type == QString::fromUtf8("FitBH")) {
      if (fz_array_len(dest) != 3)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        top = fz_to_real(obj);
      else if (fz_is_null(obj))
        top = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitBH);
      retVal.setRect(QRectF(QPointF(-1, top), QPointF(-1, -1)));
    }
    // /FitBV left
    else if (type == QString::fromUtf8("FitBV")) {
      if (fz_array_len(dest) != 3)
        return PDFDestination();
      obj = fz_array_get(dest, 2);
      if (fz_is_real(obj) || fz_is_int(obj))
        left = fz_to_real(obj);
      else if (fz_is_null(obj))
        left = -1;
      else
        return PDFDestination();
      retVal.setType(PDFDestination::Destination_FitBV);
      retVal.setRect(QRectF(QPointF(left, -1), QPointF(-1, -1)));
    }

    return retVal;
  }

  return PDFDestination();
}

#ifdef DEBUG
  const char * fz_type(fz_obj *obj) {
    if (!obj)
      return "(NULL)";
    if (fz_is_null(obj))
      return "null";
    if (fz_is_bool(obj))
      return "bool";
    if (fz_is_int(obj))
      return "int";
    if (fz_is_real(obj))
      return "real";
    if (fz_is_name(obj))
      return "name";
    if (fz_is_string(obj))
      return "string";
    if (fz_is_array(obj))
      return "array";
    if (fz_is_dict(obj))
      return "dict";
    if (fz_is_indirect(obj))
      return "reference";
    return "(unknown)";
  }

#endif


// Document Class
// ==============
MuPDFDocument::MuPDFDocument(QString fileName):
  Super(fileName),
  _glyph_cache(fz_new_glyph_cache())
{
  // NOTE: The next two calls can fail---we need to check for that
  fz_stream *pdf_file = fz_open_file(fileName.toLocal8Bit().data());
  pdf_open_xref_with_stream(&_mupdf_data, pdf_file, NULL);
  fz_close(pdf_file);

  // NOTE: This can also fail.
  pdf_load_page_tree(_mupdf_data);

  _numPages = pdf_count_pages(_mupdf_data);


  loadMetaData();
}

MuPDFDocument::~MuPDFDocument()
{
  if( _mupdf_data ){
    pdf_free_xref(_mupdf_data);
    _mupdf_data = NULL;
  }

  fz_free_glyph_cache(_glyph_cache);
}

QSharedPointer<Page> MuPDFDocument::page(int at)
{
  // FIXME: Come up with something to deal with a zero-page PDF.
  assert(_numPages != 0);

  if( _pages.isEmpty() )
    _pages.resize(_numPages);

  if( _pages[at].isNull() )
    _pages[at] = QSharedPointer<Page>(new MuPDFPage(this, at));

  return QSharedPointer<Page>(_pages[at]);
}

void MuPDFDocument::loadMetaData()
{
  char infoName[] = "Info"; // required because fz_dict_gets is not prototyped to take const char *

  // Note: fz_is_dict(NULL)===0, i.e., it doesn't crash
  if (!fz_is_dict(_mupdf_data->trailer))
    return;
  fz_obj * info = fz_dict_gets(_mupdf_data->trailer, infoName);
  if (fz_is_dict(info)) { // the `Info` entry is optional
    for (int i = 0; i < fz_dict_len(info); ++i) {
      // TODO: Check if fromAscii always gives correct results (the pdf specs
      // allow for arbitrary 8bit characters - fromAscii can handle them, but
      // can be codec dependent (see Qt docs)
      // Note: fz_to_name returns an internal pointer that must not be freed
      QString key = QString::fromAscii(fz_to_name(fz_dict_get_key(info, i)));
      // Handle standard keys
      if (key == QString::fromUtf8("Trapped")) {
        // TODO: Check if fromAscii always gives correct results (the pdf specs
        // allow for arbitrary 8bit characters - fromAscii can handle them, but
        // can be codec dependent (see Qt docs)
        QString val = QString::fromAscii(fz_to_name(fz_dict_get_val(info, i)));
        if (val == QString::fromUtf8("True"))
          _meta_trapped = Trapped_True;
        else if (val == QString::fromUtf8("False"))
          _meta_trapped = Trapped_False;
        else
          _meta_trapped = Trapped_Unknown;
      }
      else {
        // TODO: Check if fromAscii always gives correct results (the pdf specs
        // allow for arbitrary 8bit characters - fromAscii can handle them, but
        // can be codec dependent (see Qt docs)
        QString val = QString::fromAscii(fz_to_str_buf(fz_dict_get_val(info, i)));
        if (key == QString::fromUtf8("Title"))
          _meta_title = val;
        else if (key == QString::fromUtf8("Author"))
          _meta_author = val;
        else if (key == QString::fromUtf8("Subject"))
          _meta_subject = val;
        else if (key == QString::fromUtf8("Keywords"))
          _meta_keywords = val;
        else if (key == QString::fromUtf8("Creator"))
          _meta_creator = val;
        else if (key == QString::fromUtf8("Producer"))
          _meta_producer = val;
        else if (key == QString::fromUtf8("CreationDate"))
          _meta_creationDate = fromPDFDate(val);
        else if (key == QString::fromUtf8("ModDate"))
          _meta_modDate = fromPDFDate(val);
        else
          _meta_other[key] = val;
      }
    }
  }
  // FIXME: Implement metadata stream handling (which should probably override
  // the data in the `Info` dictionary
}

PDFDestination MuPDFDocument::resolveDestination(const PDFDestination & namedDestination) const
{
  Q_ASSERT(_mupdf_data != NULL);
  
  // TODO: Test this method

  // If namedDestination is not a named destination at all, simply return a copy
  if (namedDestination.isExplicit())
    return namedDestination;

  fz_obj * name = fz_new_name(namedDestination.destinationName().toUtf8().data());
  // Note: Ideally, we would use resolve_dest, but that is not declared
  // officially in mupdf.h, only in <mupdf>/pdf/pdf_annot.c
  //fz_obj * dest = resolve_dest(_mupdf_data, name)
  fz_obj * dest = pdf_lookup_dest(_mupdf_data, name);
  fz_drop_obj(name);
  return toPDFDestination(_mupdf_data, dest);
}

QList<PDFFontInfo> MuPDFDocument::fonts() const
{
  int i;
  char typeKey[] = "Type";
  char subtypeKey[] = "Subtype";
  char descriptorKey[] = "FontDescriptor";
  char basefontKey[] = "BaseFont";
  char nameKey[] = "Name";
  char fontnameKey[] = "FontName";
  char fontfileKey[] = "FontFile";
  char fontfile2Key[] = "FontFile2";
  char fontfile3Key[] = "FontFile3";
  QList<PDFFontInfo> retVal;

#ifdef DEBUG
  QTime timer;
  timer.start();
#endif

  // Iterate over all objects
  for (i = 0; i < _mupdf_data->len; ++i) {
    switch (_mupdf_data->table[i].type) {
      case 'o':
      case 'n':
      {
        if (!fz_is_dict(_mupdf_data->table[i].obj))
          continue;
        if (QString::fromAscii(fz_to_name(fz_dict_gets(_mupdf_data->table[i].obj, typeKey))) != QString::fromUtf8("Font"))
          continue;

        QString subtype = fz_to_name(fz_dict_gets(_mupdf_data->table[i].obj, subtypeKey));

        // Type0 fonts have no info we need right now---all relevant data is in
        // its descendant, which again is a dict of type /Font
        if (subtype == QString::fromUtf8("Type0"))
          continue;

        PDFFontDescriptor descriptor;
        PDFFontInfo fi;
        
        // Parse the /FontDescriptor dictionary (if it exists)
        // If not, try to derive the font name from /BaseFont (if it exists) or
        // /Name
        fz_obj * desc = fz_dict_gets(_mupdf_data->table[i].obj, descriptorKey);
        if (fz_is_dict(desc)) {
          descriptor.setName(QString::fromAscii(fz_to_name(fz_dict_gets(desc, fontnameKey))));
          fz_obj * ff = fz_dict_gets(desc, fontfileKey);
          if (fz_is_dict(ff)) {
            fi.setFontProgramType(PDFFontInfo::ProgramType_Type1);
            fi.setSource(PDFFontInfo::Source_Embedded);
          }
          else {
            fz_obj * ff = fz_dict_gets(desc, fontfile2Key);
            if (fz_is_dict(ff)) {
              fi.setFontProgramType(PDFFontInfo::ProgramType_TrueType);
              fi.setSource(PDFFontInfo::Source_Embedded);
            }
            else {
              fz_obj * ff = fz_dict_gets(desc, fontfile3Key);
              if (fz_is_dict(ff)) {
                QString ffSubtype = fz_to_name(fz_dict_gets(ff, subtypeKey));
                if (ffSubtype == QString::fromUtf8("Type1C"))
                  fi.setFontProgramType(PDFFontInfo::ProgramType_Type1CFF);
                else if (ffSubtype == QString::fromUtf8("CIDFontType0C"))
                  fi.setFontProgramType(PDFFontInfo::ProgramType_CIDCFF);
                else if (ffSubtype == QString::fromUtf8("OpenType"))
                  fi.setFontProgramType(PDFFontInfo::ProgramType_OpenType);
                fi.setSource(PDFFontInfo::Source_Embedded);
              }
              else {
                // Note: It seems MuPDF handles embedded fonts, and for everything
                // else uses its own, built-in base14 fonts as substitution (i.e., it
                // doesn't use fonts from the file system)
                fi.setFontProgramType(PDFFontInfo::ProgramType_None);
                fi.setSource(PDFFontInfo::Source_Builtin);
              }
            }
          }
          // TODO: Parse other entries in /FontDescriptor if ever we need them
        }
        else {
          // Note: It seems MuPDF handles embedded fonts, and for everything
          // else uses its own, built-in base14 fonts as substitution (i.e., it
          // doesn't use fonts from the file system)
          fi.setFontProgramType(PDFFontInfo::ProgramType_None);
          fi.setSource(PDFFontInfo::Source_Builtin);

          fz_obj * basefont = fz_dict_gets(_mupdf_data->table[i].obj, basefontKey);
          if (fz_is_name(basefont))
            descriptor.setName(QString::fromAscii(fz_to_name(basefont)));
          else
            descriptor.setName(QString::fromAscii(fz_to_name(fz_dict_gets(_mupdf_data->table[i].obj, nameKey))));
        }
        fi.setDescriptor(descriptor);

        // Set the font type
        if (subtype == QString::fromUtf8("Type1")) {
          fi.setFontType(PDFFontInfo::FontType_Type1);
          fi.setCIDType(PDFFontInfo::CIDFont_None);
        }
        else if (subtype == QString::fromUtf8("MMType1")) {
          fi.setFontType(PDFFontInfo::FontType_MMType1);
          fi.setCIDType(PDFFontInfo::CIDFont_None);
        }
        else if (subtype == QString::fromUtf8("Type3")) {
          fi.setFontType(PDFFontInfo::FontType_Type3);
          fi.setCIDType(PDFFontInfo::CIDFont_None);
        }
        else if (subtype == QString::fromUtf8("TrueType")) {
          fi.setFontType(PDFFontInfo::FontType_TrueType);
          fi.setCIDType(PDFFontInfo::CIDFont_None);
        }
        else if (subtype == QString::fromUtf8("CIDFontType0")) {
          fi.setFontType(PDFFontInfo::FontType_Type0);
          fi.setCIDType(PDFFontInfo::CIDFont_Type0);
        }
        else if (subtype == QString::fromUtf8("CIDFontType2")) {
          fi.setFontType(PDFFontInfo::FontType_Type0);
          fi.setCIDType(PDFFontInfo::CIDFont_Type2);
        }

        retVal << fi;
        break;
      }
      case 0:
      case 'f':
      default:
        continue;
    }
  }

#ifdef DEBUG
  qDebug() << "loaded fonts in" << timer.elapsed() << "ms";
#endif

  return retVal;
}

void MuPDFDocument::recursiveConvertToC(QList<PDFToCItem> & items, pdf_outline * node) const
{
  while (node && node->title) {
    // TODO: It seems that this works, at least for pdfs produced with pdflatex
    // using either utf8 or latin1 encoding (and the approrpriate inputenc
    // package). Is this valid generally?
    PDFToCItem item(QString::fromUtf8(node->title));
    item.setOpen(node->count > 0);

    if (node->link && (node->link->kind == PDF_LINK_GOTO || node->link->kind == PDF_LINK_NAMED)) {
      Q_ASSERT(_mupdf_data != NULL);
      item.setAction(new PDFGotoAction(toPDFDestination(_mupdf_data, node->link->dest)));
    }

    recursiveConvertToC(item.children(), node->child);

    // NOTE: pdf_outline doesn't include color or flags; we could go through the
    // pdf ourselves to get to them, but for now we simply don't support them
    items << item;
    node = node->next;
  }
}

PDFToC MuPDFDocument::toc() const
{
  Q_ASSERT(_mupdf_data != NULL);
  PDFToC retVal;
  pdf_outline * outline = pdf_load_outline(_mupdf_data);
  recursiveConvertToC(retVal, outline);
  pdf_free_outline(outline);
  return retVal;
}

// Page Class
// ==========
MuPDFPage::MuPDFPage(MuPDFDocument *parent, int at):
  Super(parent, at),
  _linksLoaded(false)
{
  pdf_page *page_data;
  pdf_load_page(&page_data, parent->_mupdf_data, _n);

  _bbox = page_data->mediabox;
  _size = QSizeF(qreal(_bbox.x1 - _bbox.x0), qreal(_bbox.y1 - _bbox.y0));
  _rotate = qreal(page_data->rotate);

  // This is also time-intensive. It takes Poppler ~500 ms to create page
  // objects for the entire PGF Manual. MuPDF takes ~1000 ms, but only ~200 if
  // this step is omitted. May be useful to move this into a render function
  // but it will require us to keep page_data around.
  _mupdf_page = fz_new_display_list();
  fz_device *dev = fz_new_list_device(_mupdf_page);
  pdf_run_page(parent->_mupdf_data, page_data, dev, fz_identity);

  fz_free_device(dev);
  pdf_free_page(page_data);
}

MuPDFPage::~MuPDFPage()
{
  if( _mupdf_page )
    fz_free_display_list(_mupdf_page);
  _mupdf_page = NULL;
}

QSizeF MuPDFPage::pageSizeF() { return _size; }

QImage MuPDFPage::renderToImage(double xres, double yres, QRect render_box, bool cache)
{
  // Set up the transformation matrix for the page. Really, we just start with
  // an identity matrix and scale it using the xres, yres inputs.
  fz_matrix render_trans = fz_identity;
  render_trans = fz_concat(render_trans, fz_translate(0, -_bbox.y1));
  render_trans = fz_concat(render_trans, fz_scale(xres/72.0, -yres/72.0));
  render_trans = fz_concat(render_trans, fz_rotate(_rotate));


  qDebug() << "Page bbox is: (" << _bbox.x0 << "," << _bbox.y0 << "|" << _bbox.x1 << "," << _bbox.y1 << ")";

  fz_bbox render_bbox;
  if ( not render_box.isNull() ) {
    render_bbox.x0 = render_box.left();
    render_bbox.y0 = render_box.top();
    render_bbox.x1 = render_box.right();
    render_bbox.y1 = render_box.bottom();
  } else {
    render_bbox = fz_round_rect(fz_transform_rect(render_trans, _bbox));
  }

  qDebug() << "Render bbox is: (" << render_bbox.x0 << "," << render_bbox.y0 << "|" << render_bbox.x1 << "," << render_bbox.y1 << ")";

  // NOTE: Using fz_device_bgr or fz_device_rbg may depend on platform endianness.
  fz_pixmap *mu_image = fz_new_pixmap_with_rect(fz_device_bgr, render_bbox);
  // Flush to white.
  fz_clear_pixmap_with_color(mu_image, 255);
  fz_device *renderer = fz_new_draw_device(static_cast<MuPDFDocument *>(_parent)->_glyph_cache, mu_image);

  // Actually render the page.
  fz_execute_display_list(_mupdf_page, renderer, render_trans, render_bbox);

  // Create a QImage that shares data with the fz_pixmap.
  QImage tmp_image(mu_image->samples, mu_image->w, mu_image->h, QImage::Format_ARGB32);
  // Now create a copy with its own data that can exist outside this function
  // call.
  QImage renderedPage = tmp_image.copy();

  // Dispose of unneeded items.
  fz_free_device(renderer);
  fz_drop_pixmap(mu_image);

  if( cache ) {
    PDFPageTile key(xres, yres, render_box, _n);
    QImage * img = new QImage(renderedPage.copy());
    if (img != _parent->pageCache().setImage(key, img))
      delete img;
  }

  return renderedPage;
}

QList< QSharedPointer<PDFLinkAnnotation> > MuPDFPage::loadLinks()
{
  if (_linksLoaded)
    return _links;

  Q_ASSERT(_parent != NULL);
  pdf_xref * xref = static_cast<MuPDFDocument*>(_parent)->_mupdf_data;
  Q_ASSERT(xref != NULL);

  // FIXME: PDFLinkGraphicsItem erroneously requires coordinates to be
  // normalized to [0..1].
  QTransform normalize = QTransform::fromScale(1. / _size.width(), -1. / _size.height()).translate(0, -_size.height());

  _linksLoaded = true;
  pdf_page * page;
  if (pdf_load_page(&page, xref, _n) != fz_okay)
    return _links;

  if (!page)
    return _links;
  pdf_link * mupdfLink = page->links;

  while (mupdfLink) {
    QSharedPointer<PDFLinkAnnotation> link(new PDFLinkAnnotation);
    link->setRect(normalize.mapRect(toRectF(mupdfLink->rect)));
    link->setPage(this);
    // FIXME: Initialize all other properties of PDFLinkAnnotation, such as
    // border, color, quadPoints, etc.

    switch (mupdfLink->kind) {
      case PDF_LINK_NAMED:
      case PDF_LINK_GOTO:
        link->setActionOnActivation(new PDFGotoAction(toPDFDestination(xref, mupdfLink->dest)));
        break;
      case PDF_LINK_URI:
        // TODO: Check if MuPDF indeed always returns properly encoded URLs
        link->setActionOnActivation(new PDFURIAction(QUrl::fromEncoded(fz_to_str_buf(mupdfLink->dest))));
        break;
      case PDF_LINK_LAUNCH:
        // TODO: Check if fromLocal8Bit works in all cases. The pdf specs say
        // that "[file specs] are stored as bytes and are passed to the
        // operating system without interpretation or conversion of any sort."
        // Since we have no influence on how strings are treated in process
        // creation, we cannot fully comply with this. However, it seems that
        // (at least in Qt 4.7.2):
        //  - on Unix/Mac, QFile::encodeName is used; Qt docs say that "this
        //    function converts fileName to the local 8-bit encoding determined
        //    by the user's locale".
        //  - on Win, full utf16 is used
        link->setActionOnActivation(new PDFLaunchAction(QString::fromLocal8Bit(fz_to_str_buf(mupdfLink->dest))));
        break;
      case PDF_LINK_ACTION:
        // we don't handle this yet
        link.clear();
        break;
    }
    if (link)
      _links << link;
    mupdfLink = mupdfLink->next;
  }

  pdf_free_page(page);
  return _links;
}

QList<SearchResult> MuPDFPage::search(QString searchText)
{
  // FIXME: Currently unimplemented and always returns an empty list.
  QList<SearchResult> results;

  return results;
}


// vim: set sw=2 ts=2 et

