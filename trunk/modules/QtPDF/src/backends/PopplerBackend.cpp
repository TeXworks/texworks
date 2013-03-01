/**
 * Copyright (C) 2011  Charlie Sharpsteen, Stefan Löffler
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

// NOTE: `PopplerBackend.h` is included via `PDFBackend.h`
#include <PDFBackend.h>

// TODO: Find a better place to put this
PDFDestination toPDFDestination(const Poppler::Document * doc, const Poppler::LinkDestination & dest)
{
  if (!dest.destinationName().isEmpty())
    return PDFDestination(dest.destinationName());

  // Coordinates in LinkDestination are in the range of 0..1, which does not
  // comply with the pdf specs---so we have to convert them back
  float w = 1., h = 1.;
  if (doc) {
    Poppler::Page * p = doc->page(dest.pageNumber() - 1);
    if (p) {
      w = p->pageSizeF().width();
      h = p->pageSizeF().height();
    }
  }

  PDFDestination retVal(dest.pageNumber() - 1);
  // Note: Poppler seems to give coordinates in a vertically mirrored coordinate
  // system, i.e., y=0 corresponds to the top of the page. This does not
  // comply with the pdf page coordinate system, which has y=0 at the bottom. So
  // we need to compensate for that.
  switch (dest.kind()) {
    case Poppler::LinkDestination::destXYZ:
      retVal.setType(PDFDestination::Destination_XYZ);
      retVal.setRect(QRectF((dest.isChangeLeft() ? dest.left() * w : -1), (dest.isChangeTop() ? (1 - dest.top()) * h : -1), -1, -1));
      retVal.setZoom((dest.isChangeZoom() ? dest.zoom() : -1));
      break;
    case Poppler::LinkDestination::destFit:
      retVal.setType(PDFDestination::Destination_Fit);
      break;
    case Poppler::LinkDestination::destFitH:
      retVal.setType(PDFDestination::Destination_FitH);
      retVal.setRect(QRectF(-1, (dest.isChangeTop() ? (1 - dest.top()) * h : -1), -1, -1));
      break;
    case Poppler::LinkDestination::destFitV:
      retVal.setType(PDFDestination::Destination_FitV);
      retVal.setRect(QRectF((dest.isChangeLeft() ? dest.left() * w : -1), -1, -1, -1));
      break;
    case Poppler::LinkDestination::destFitR:
      retVal.setType(PDFDestination::Destination_FitR);
      retVal.setRect(QRectF(QPointF(dest.left() * w, (1 - dest.top()) * h), QPointF(dest.right() * w, dest.bottom() * h)));
      break;
    case Poppler::LinkDestination::destFitB:
      retVal.setType(PDFDestination::Destination_FitB);
      break;
    case Poppler::LinkDestination::destFitBH:
      retVal.setType(PDFDestination::Destination_FitBH);
      retVal.setRect(QRectF(-1, (dest.isChangeTop() ? (1 - dest.top()) * h : -1), -1, -1));
      break;
    case Poppler::LinkDestination::destFitBV:
      retVal.setType(PDFDestination::Destination_FitBV);
      retVal.setRect(QRectF((dest.isChangeLeft() ? dest.left() * w : -1), -1, -1, -1));
      break;
  }
  return retVal;
}

void convertAnnotation(PDFAnnotation * dest, const Poppler::Annotation * src, Page * page)
{
  if (!dest || !src || !page)
    return;

  QTransform denormalize = QTransform::fromScale(page->pageSizeF().width(), -page->pageSizeF().height()).translate(0,  -1);

  dest->setRect(denormalize.mapRect(src->boundary()));
  dest->setContents(src->contents());
  dest->setName(src->uniqueName());
  dest->setLastModified(src->modificationDate());
  dest->setPage(page);

  // TODO: Does poppler provide the color anywhere?
  // dest->setColor();

  QFlags<PDFAnnotation::AnnotationFlags>& flags = dest->flags();
  flags = QFlags<PDFAnnotation::AnnotationFlags>();
  if (src->flags() & Poppler::Annotation::Hidden)
    flags |= PDFAnnotation::Annotation_Hidden;
  if (src->flags() & Poppler::Annotation::FixedSize)
    flags |= PDFAnnotation::Annotation_NoZoom;
  if (src->flags() & Poppler::Annotation::FixedRotation)
    flags |= PDFAnnotation::Annotation_NoRotate;
  if ((src->flags() & Poppler::Annotation::DenyPrint) == 0)
    flags |= PDFAnnotation::Annotation_Print;
  if (src->flags() & Poppler::Annotation::DenyWrite)
    flags |= PDFAnnotation::Annotation_ReadOnly;
  if (src->flags() & Poppler::Annotation::DenyDelete)
    flags |= PDFAnnotation::Annotation_Locked;
  if (src->flags() & Poppler::Annotation::ToggleHidingOnMouse)
    flags |= PDFAnnotation::Annotation_ToggleNoView;
  
  if (dest->isMarkup()) {
    PDFMarkupAnnotation * annot = static_cast<PDFMarkupAnnotation*>(dest);
    annot->setAuthor(src->author());
    annot->setCreationDate(src->creationDate());
  }
}


// Document Class
// ==============
PopplerDocument::PopplerDocument(QString fileName):
  Super(fileName),
  _poppler_doc(Poppler::Document::load(fileName)),
  _doc_lock(new QMutex())
{
  parseDocument();
}

PopplerDocument::~PopplerDocument()
{
}

void PopplerDocument::parseDocument()
{
  if (!_poppler_doc || isLocked())
    return;

  _numPages = _poppler_doc->numPages();

  // Permissions
  // TODO: Check if this mapping from Poppler flags to our flags is correct
  if (_poppler_doc->okToAddNotes())
    _permissions |= Permission_Annotate;
  if (_poppler_doc->okToAssemble())
    _permissions |= Permission_Assemble;
  if (_poppler_doc->okToChange())
    _permissions |= Permission_Change;
  if (_poppler_doc->okToCopy())
    _permissions |= Permission_Extract;
  if (_poppler_doc->okToCreateFormFields())
    _permissions |= Permission_Annotate;
  if (_poppler_doc->okToExtractForAccessibility())
    _permissions |= Permission_ExtractForAccessibility;
  if (_poppler_doc->okToFillForm())
    _permissions |= Permission_FillForm;
  if (_poppler_doc->okToPrint())
    _permissions |= Permission_Print;
  if (_poppler_doc->okToPrintHighRes())
    _permissions |= Permission_PrintHighRes;

  // **TODO:**
  //
  // _Make these configurable._
  _poppler_doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  _poppler_doc->setRenderHint(Poppler::Document::Antialiasing);
  _poppler_doc->setRenderHint(Poppler::Document::TextAntialiasing);

  // Load meta data
  QStringList metaKeys = _poppler_doc->infoKeys();
  if (metaKeys.contains(QString::fromUtf8("Title"))) {
    _meta_title = _poppler_doc->info(QString::fromUtf8("Title"));
    metaKeys.removeAll(QString::fromUtf8("Title"));
  }
  if (metaKeys.contains(QString::fromUtf8("Author"))) {
    _meta_author = _poppler_doc->info(QString::fromUtf8("Author"));
    metaKeys.removeAll(QString::fromUtf8("Author"));
  }
  if (metaKeys.contains(QString::fromUtf8("Subject"))) {
    _meta_subject = _poppler_doc->info(QString::fromUtf8("Subject"));
    metaKeys.removeAll(QString::fromUtf8("Subject"));
  }
  if (metaKeys.contains(QString::fromUtf8("Keywords"))) {
    _meta_keywords = _poppler_doc->info(QString::fromUtf8("Keywords"));
    metaKeys.removeAll(QString::fromUtf8("Keywords"));
  }
  if (metaKeys.contains(QString::fromUtf8("Creator"))) {
    _meta_creator = _poppler_doc->info(QString::fromUtf8("Creator"));
    metaKeys.removeAll(QString::fromUtf8("Creator"));
  }
  if (metaKeys.contains(QString::fromUtf8("Producer"))) {
    _meta_producer = _poppler_doc->info(QString::fromUtf8("Producer"));
    metaKeys.removeAll(QString::fromUtf8("Producer"));
  }
  if (metaKeys.contains(QString::fromUtf8("CreationDate"))) {
    _meta_creationDate = fromPDFDate(_poppler_doc->info(QString::fromUtf8("CreationDate")));
    metaKeys.removeAll(QString::fromUtf8("CreationDate"));
  }
  if (metaKeys.contains(QString::fromUtf8("ModDate"))) {
    _meta_modDate = fromPDFDate(_poppler_doc->info(QString::fromUtf8("ModDate")));
    metaKeys.removeAll(QString::fromUtf8("ModDate"));
  }

  // Note: Poppler doesn't handle the meta data key "Trapped" correctly, as that
  // has a value of type `name` (/True, /False, or /Unknown) which doesn't get
  // converted to a string representation properly.
  _meta_trapped = Trapped_Unknown;
    metaKeys.removeAll(QString::fromUtf8("Trapped"));
  
  foreach (QString key, metaKeys)
    _meta_other[key] = _poppler_doc->info(key);
}

QSharedPointer<Page> PopplerDocument::page(int at)
{
  if (at < 0 || at >= _numPages)
    return QSharedPointer<Page>();

  if( _pages.isEmpty() )
    _pages.resize(_numPages);

  if( _pages[at].isNull() )
    _pages[at] = QSharedPointer<Page>(new PopplerPage(this, at));

  return QSharedPointer<Page>(_pages[at]);
}

PDFDestination PopplerDocument::resolveDestination(const PDFDestination & namedDestination) const
{
  Q_ASSERT(!_poppler_doc.isNull());

  // If namedDestination is not a named destination at all, simply return a copy
  if (namedDestination.isExplicit())
    return namedDestination;

  // If the destination could not be resolved, return an invalid object
  Poppler::LinkDestination * dest = _poppler_doc->linkDestination(namedDestination.destinationName());
  if (!dest)
    return PDFDestination();
  return toPDFDestination(_poppler_doc.data(), *dest);
}

void PopplerDocument::recursiveConvertToC(QList<PDFToCItem> & items, QDomNode node) const
{
  while (!node.isNull()) {
    PDFToCItem newItem(node.nodeName());

    QDomNamedNodeMap attributes = node.attributes();
    newItem.setOpen(attributes.namedItem(QString::fromUtf8("Open")).nodeValue() == QString::fromUtf8("true"));
    // Note: color and flags are not supported by poppler

    PDFGotoAction * action = NULL;
    QString val = attributes.namedItem(QString::fromUtf8("Destination")).nodeValue();
    if (!val.isEmpty())
      action = new PDFGotoAction(toPDFDestination(_poppler_doc.data(), Poppler::LinkDestination(val)));
    else {
      val = attributes.namedItem(QString::fromUtf8("DestinationName")).nodeValue();
      if (!val.isEmpty())
        action = new PDFGotoAction(PDFDestination(val));
    }

    val = attributes.namedItem(QString::fromUtf8("ExternalFileName")).nodeValue();
    if (action) {
      if (!val.isEmpty()) {
        // Open external links in new window by default (since poppler doesn't
        // tell us what to do)
        action->setOpenInNewWindow(true);
        action->setRemote();
        action->setFilename(val);
      }
    }
    newItem.setAction(action);

    recursiveConvertToC(newItem.children(), node.firstChild());
    items << newItem;
    node = node.nextSibling();
  }
}

PDFToC PopplerDocument::toc() const
{
  PDFToC retVal;
  if (!_poppler_doc || isLocked())
    return retVal;

  QDomDocument * toc = _poppler_doc->toc();
  if (!toc)
    return retVal;
  recursiveConvertToC(retVal, toc->firstChild());
  delete toc;
  return retVal;
}

QList<PDFFontInfo> PopplerDocument::fonts() const
{
  QList<PDFFontInfo> retVal;
  if (!_poppler_doc || isLocked())
    return retVal;

  foreach(Poppler::FontInfo popplerFontInfo, _poppler_doc->fonts()) {
    PDFFontInfo fi;
    if (popplerFontInfo.isEmbedded())
      fi.setSource(PDFFontInfo::Source_Embedded);
    else
      fi.setFileName(popplerFontInfo.file());
    fi.setDescriptor(PDFFontDescriptor(popplerFontInfo.name()));

    switch (popplerFontInfo.type()) {
      case Poppler::FontInfo::Type1:
        fi.setFontType(PDFFontInfo::FontType_Type1);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_Type1);
        break;
      case Poppler::FontInfo::Type1C:
        fi.setFontType(PDFFontInfo::FontType_Type1);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_Type1CFF);
        break;
      case Poppler::FontInfo::Type1COT:
        fi.setFontType(PDFFontInfo::FontType_Type1);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_OpenType); // speculation
        break;
      case Poppler::FontInfo::Type3:
        fi.setFontType(PDFFontInfo::FontType_Type3);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_None); // probably wrong!
        break;
      case Poppler::FontInfo::TrueType:
        fi.setFontType(PDFFontInfo::FontType_TrueType);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_TrueType);
        break;
      case Poppler::FontInfo::TrueTypeOT:
        fi.setFontType(PDFFontInfo::FontType_TrueType);
        fi.setCIDType(PDFFontInfo::CIDFont_None);
        fi.setFontProgramType(PDFFontInfo::ProgramType_OpenType);
        break;
      case Poppler::FontInfo::CIDType0:
        fi.setFontType(PDFFontInfo::FontType_Type0);
        fi.setCIDType(PDFFontInfo::CIDFont_Type0);
        fi.setFontProgramType(PDFFontInfo::ProgramType_None); // probably wrong!
        break;
      case Poppler::FontInfo::CIDType0C:
        fi.setFontType(PDFFontInfo::FontType_Type0);
        fi.setCIDType(PDFFontInfo::CIDFont_Type0);
        fi.setFontProgramType(PDFFontInfo::ProgramType_CIDCFF);
        break;
      case Poppler::FontInfo::CIDType0COT:
        fi.setFontType(PDFFontInfo::FontType_Type0);
        fi.setCIDType(PDFFontInfo::CIDFont_Type0);
        fi.setFontProgramType(PDFFontInfo::ProgramType_OpenType);
        break;
      case Poppler::FontInfo::CIDTrueType:
        fi.setFontType(PDFFontInfo::FontType_Type0);
        fi.setCIDType(PDFFontInfo::CIDFont_Type2); // speculation
        fi.setFontProgramType(PDFFontInfo::ProgramType_TrueType);
        break;
      case Poppler::FontInfo::CIDTrueTypeOT:
        fi.setFontType(PDFFontInfo::FontType_Type0);
        fi.setCIDType(PDFFontInfo::CIDFont_Type2); // speculation
        fi.setFontProgramType(PDFFontInfo::ProgramType_OpenType);
        break;
      case Poppler::FontInfo::unknown:
      default:
        continue;
    }
    retVal << fi;
  }
  return retVal;
}

bool PopplerDocument::unlock(const QString password)
{
  if (!_poppler_doc)
    return false;
  // Note: we try unlocking regardless of what isLocked() returns as the user
  // might want to unlock a document with the owner's password when user level
  // access is already granted.
  bool success = !_poppler_doc->unlock(password.toLatin1(), password.toLatin1());

  if (success)
    parseDocument();

  // FIXME: Store password for this session in case we need to reload the
  // document later on (e.g., if it has changed on the disk)

  return success;
}


// Page Class
// ==========
PopplerPage::PopplerPage(PopplerDocument *parent, int at):
  Super(parent, at),
  _annotationsLoaded(false),
  _linksLoaded(false)
{
  _poppler_page = QSharedPointer<Poppler::Page>(static_cast<PopplerDocument *>(_parent)->_poppler_doc->page(at));
}

PopplerPage::~PopplerPage()
{
}

// TODO: Does this operation require obtaining the Poppler document mutex? If
// so, it would be better to store the value in a member variable during
// initialization.
QSizeF PopplerPage::pageSizeF() const {
  Q_ASSERT(_poppler_page != NULL);
  return _poppler_page->pageSizeF();
}

QImage PopplerPage::renderToImage(double xres, double yres, QRect render_box, bool cache)
{
  QImage renderedPage;

  // Rendering pages is not thread safe.
  QMutexLocker docLock(static_cast<PopplerDocument *>(_parent)->_doc_lock);
    if( render_box.isNull() ) {
      // A null QRect has a width and height of 0 --- we will tell Poppler to render the whole
      // page.
      renderedPage = _poppler_page->renderToImage(xres, yres);
    } else {
      renderedPage = _poppler_page->renderToImage(xres, yres,
          render_box.x(), render_box.y(), render_box.width(), render_box.height());
    }
  docLock.unlock();

  if( cache ) {
    PDFPageTile key(xres, yres, render_box, _n);
    QImage * img = new QImage(renderedPage.copy());
    if (img != _parent->pageCache().setImage(key, img))
      delete img;
  }

  return renderedPage;
}

QList< QSharedPointer<PDFLinkAnnotation> > PopplerPage::loadLinks()
{
  if (_linksLoaded)
    return _links;

  _linksLoaded = true;
  // Loading links is not thread safe.
  QMutexLocker docLock(static_cast<PopplerDocument *>(_parent)->_doc_lock);
  QList<Poppler::Link *> popplerLinks = _poppler_page->links();
  QList<Poppler::Annotation *> popplerAnnots = _poppler_page->annotations();
  docLock.unlock();

  // Note: Poppler gives the linkArea in normalized coordinates, i.e., in the
  // range of 0..1, with y=0 at the top. We use pdf coordinates internally, so
  // we need to transform things accordingly.
  QTransform denormalize = QTransform::fromScale(pageSizeF().width(), -pageSizeF().height()).translate(0,  -1);

  // Convert poppler links to PDFLinkAnnotations
  foreach (Poppler::Link * popplerLink, popplerLinks) {
    QSharedPointer<PDFLinkAnnotation> link(new PDFLinkAnnotation);

    // Look up the corresponding Poppler::LinkAnnotation object. Do this first
    // so the general annotation settings can be overridden by more specific
    // link annotation settings afterwards (if necessary)
    // Note: Poppler::LinkAnnotation::linkDestionation() [sic] doesn't reliably
    // return a Poppler::Link*. Therefore, we have to find the correct
    // annotation object ourselves. Note, though, that boundary() and rect()
    // don't seem to correspond exactly (i.e., they are neither (necessarily)
    // equal, nor does one (necessarily) contain the other.
    // TODO: Can we have the situation that we get more than one matching
    // annotations out of this?
    foreach (Poppler::Annotation * popplerAnnot, popplerAnnots) {
      if (!popplerAnnot || popplerAnnot->subType() != Poppler::Annotation::ALink || !denormalize.mapRect(popplerAnnot->boundary()).intersects(link->rect()))
        continue;

      Poppler::LinkAnnotation * popplerLinkAnnot = static_cast<Poppler::LinkAnnotation *>(popplerAnnot);
      convertAnnotation(link.data(), popplerLinkAnnot, this);
      // TODO: Does Poppler provide an easy interface to all quadPoints?
      // Note: Poppler::LinkAnnotation::HighlightMode is identical to PDFLinkAnnotation::HighlightingMode
      link->setHighlightingMode((PDFLinkAnnotation::HighlightingMode)popplerLinkAnnot->linkHighlightMode());
      break;
    }

    link->setRect(denormalize.mapRect(popplerLink->linkArea()));
    
    switch (popplerLink->linkType()) {
      case Poppler::Link::Goto:
        {
          Poppler::LinkGoto * popplerGoto = static_cast<Poppler::LinkGoto *>(popplerLink);
          PDFGotoAction * action = new PDFGotoAction(toPDFDestination(static_cast<PopplerDocument *>(_parent)->_poppler_doc.data(), popplerGoto->destination()));
          if (popplerGoto->isExternal()) {
            // TODO: Verify that Poppler::LinkGoto only refers to pdf files
            // (for other file types we would need PDFLaunchAction)
            action->setOpenInNewWindow(true);
            action->setRemote();
            action->setFilename(popplerGoto->fileName());
          }
          link->setActionOnActivation(action);
        }
        break;
      case Poppler::Link::Execute:
        {
          Poppler::LinkExecute * popplerExecute = static_cast<Poppler::LinkExecute *>(popplerLink);
          if (popplerExecute->parameters().isEmpty())
            link->setActionOnActivation(new PDFLaunchAction(popplerExecute->fileName()));
          else
            link->setActionOnActivation(new PDFLaunchAction(QString::fromUtf8("%1 %2").arg(popplerExecute->fileName()).arg(popplerExecute->parameters())));
        }
        break;
      case Poppler::Link::Browse:
        link->setActionOnActivation(new PDFURIAction(static_cast<Poppler::LinkBrowse*>(popplerLink)->url()));
        break;
      case Poppler::Link::Action:
      case Poppler::Link::None:
      case Poppler::Link::Sound:
      case Poppler::Link::Movie:
      case Poppler::Link::JavaScript:
        // We don't handle these types yet
        link.clear();
        continue;
    }

    _links << link;
  }
  return _links;
}

QList< QSharedPointer<PDFAnnotation> > PopplerPage::loadAnnotations()
{
  if (_annotationsLoaded)
    return _annotations;

  _annotationsLoaded = true;
  if (!_poppler_page)
    return _annotations;
  
  // Loading annotations is not thread safe.
  QMutexLocker docLock(static_cast<PopplerDocument *>(_parent)->_doc_lock);
  QList<Poppler::Annotation *> popplerAnnots = _poppler_page->annotations();
  docLock.unlock();

  foreach(Poppler::Annotation * popplerAnnot, popplerAnnots) {
    if (!popplerAnnot)
      continue;
    switch (popplerAnnot->subType()) {
      case Poppler::Annotation::AText:
      {
        PDFTextAnnotation * annot = new PDFTextAnnotation();
        convertAnnotation(annot, popplerAnnot, this);
        _annotations << QSharedPointer<PDFAnnotation>(annot);
        break;
      }
      case Poppler::Annotation::ACaret:
      {
        PDFCaretAnnotation * annot = new PDFCaretAnnotation();
        convertAnnotation(annot, popplerAnnot, this);
        _annotations << QSharedPointer<PDFAnnotation>(annot);
        break;
      }
      case Poppler::Annotation::AHighlight:
      {
        Poppler::HighlightAnnotation * popplerHighlight = static_cast<Poppler::HighlightAnnotation*>(popplerAnnot);
        switch (popplerHighlight->highlightType()) {
          case Poppler::HighlightAnnotation::Highlight:
          {
            PDFHighlightAnnotation * annot = new PDFHighlightAnnotation();
            convertAnnotation(annot, popplerAnnot, this);
            _annotations << QSharedPointer<PDFAnnotation>(annot);
            break;
          }
          case Poppler::HighlightAnnotation::Squiggly:
          {
            PDFSquigglyAnnotation * annot = new PDFSquigglyAnnotation();
            convertAnnotation(annot, popplerAnnot, this);
            _annotations << QSharedPointer<PDFAnnotation>(annot);
            break;
          }
          case Poppler::HighlightAnnotation::Underline:
          {
            PDFUnderlineAnnotation * annot = new PDFUnderlineAnnotation();
            convertAnnotation(annot, popplerAnnot, this);
            _annotations << QSharedPointer<PDFAnnotation>(annot);
            break;
          }
          case Poppler::HighlightAnnotation::StrikeOut:
          {
            PDFStrikeOutAnnotation * annot = new PDFStrikeOutAnnotation();
            convertAnnotation(annot, popplerAnnot, this);
            _annotations << QSharedPointer<PDFAnnotation>(annot);
            break;
          }
        }
        break;
      }
      default:
        break;
    }
  }
  return _annotations;
}

QList<SearchResult> PopplerPage::search(QString searchText)
{
  QList<SearchResult> results;
  SearchResult result;
  double left, right, top, bottom;

  result.pageNum = _n;

  QMutexLocker docLock(static_cast<PopplerDocument *>(_parent)->_doc_lock);
    // The Poppler search function that takes a QRectF has been marked as
    // depreciated---something to do with float <-> double conversion causing
    // infinite loops on some architectures. So, we explicitly use doubles and
    // avoid the depreciated function.
    if ( _poppler_page->search(searchText, left, top, right, bottom, Poppler::Page::FromTop, Poppler::Page::CaseInsensitive) ) {
      result.bbox = QRectF(qreal(left), qreal(top), qAbs(qreal(right) - qreal(left)), qAbs(qreal(bottom) - qreal(top)));
      results << result;
    }

    while ( _poppler_page->search(searchText, left, top, right, bottom, Poppler::Page::NextResult, Poppler::Page::CaseInsensitive) ) {
      result.bbox = QRectF(qreal(left), qreal(top), qAbs(qreal(right) - qreal(left)), qAbs(qreal(bottom) - qreal(top)));
      results << result;
    }
  docLock.unlock();

  return results;
}


// vim: set sw=2 ts=2 et

