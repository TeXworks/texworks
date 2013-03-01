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

Page *MuPDFDocument::page(int at){ return new MuPDFPage(this, at); }

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
        }
        break;
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
  // FIXME: This always returns an empty list. Needs a proper implementation.
  return _links;
}


// vim: set sw=2 ts=2 et

