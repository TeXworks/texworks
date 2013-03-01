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
PDFDestination toPDFDestination(const Poppler::LinkDestination & dest)
{
  PDFDestination retVal(dest.pageNumber() - 1);
  // FIXME: viewport, zoom, fitting, etc.
  return retVal;
}



// Document Class
// ==============
PopplerDocument::PopplerDocument(QString fileName):
  Super(fileName),
  _poppler_doc(Poppler::Document::load(fileName)),
  _doc_lock(new QMutex())
{
  _numPages = _poppler_doc->numPages();

  // **TODO:**
  //
  // _Make these configurable._
  _poppler_doc->setRenderBackend(Poppler::Document::SplashBackend);
  // Make things look pretty.
  _poppler_doc->setRenderHint(Poppler::Document::Antialiasing);
  _poppler_doc->setRenderHint(Poppler::Document::TextAntialiasing);
}

PopplerDocument::~PopplerDocument()
{
}

Page *PopplerDocument::page(int at){ return new PopplerPage(this, at); }

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
      action = new PDFGotoAction(toPDFDestination(Poppler::LinkDestination(val)));
    else {
      val = attributes.namedItem(QString::fromUtf8("DestinationName")).nodeValue();
      if (!val.isEmpty() && _poppler_doc) {
        Poppler::LinkDestination * dest = _poppler_doc->linkDestination(val);
        if (dest) {
          action = new PDFGotoAction(toPDFDestination(*dest));
          delete dest;
        }
      }
    }

    val = attributes.namedItem(QString::fromUtf8("ExternalFileName")).nodeValue();
    if (action) {
      action->setOpenInNewWindow(false);
      if (!val.isEmpty()) {
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
  if (!_poppler_doc)
    return retVal;

  QDomDocument * toc = _poppler_doc->toc();
  if (!toc)
    return retVal;
  recursiveConvertToC(retVal, toc->firstChild());
  delete toc;
  return retVal;
}


// Page Class
// ==========
PopplerPage::PopplerPage(PopplerDocument *parent, int at):
  Super(parent, at),
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
QSizeF PopplerPage::pageSizeF() { return _poppler_page->pageSizeF(); }

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

  // Convert poppler links to PDFLinkAnnotations
  // Note: 
  foreach (Poppler::Link * popplerLink, popplerLinks) {
    QSharedPointer<PDFLinkAnnotation> link(new PDFLinkAnnotation);
    link->setRect(popplerLink->linkArea());
    link->setPage(this);
    
    // FIXME: Actional action/destination
    //setActionOnActivation(PDFAction * const action);
    //setDestination(PDFDestination * const destination);
    switch (popplerLink->linkType()) {
      case Poppler::Link::Goto:
        {
          Poppler::LinkGoto * popplerGoto = static_cast<Poppler::LinkGoto *>(popplerLink);
          PDFGotoAction * action = new PDFGotoAction(toPDFDestination(popplerGoto->destination()));
          if (popplerGoto->isExternal()) {
            // TODO: Verify that Poppler::LinkGoto only refers to pdf files
            // (for other file types we would need PDFLaunchAction)
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

    
    // Look up the corresponding Poppler::LinkAnnotation object
    // Note: Poppler::LinkAnnotation::linkDestionation() [sic] doesn't reliably
    // return a Poppler::Link*. Therefore, we have to find the correct
    // annotation object ourselves. Note, though, that boundary() and rect()
    // don't seem to correspond exactly (i.e., they are neither (necessarily)
    // equal, nor does one (necessarily) contain the other.
    // TODO: Can we have the situation that we get more than one matching
    // annotations out of this?
    foreach (Poppler::Annotation * popplerAnnot, popplerAnnots) {
      if (!popplerAnnot || popplerAnnot->subType() != Poppler::Annotation::ALink || !popplerAnnot->boundary().intersects(link->rect()))
        continue;
      Poppler::LinkAnnotation * popplerLinkAnnot = static_cast<Poppler::LinkAnnotation *>(popplerAnnot);
      link->setContents(popplerLinkAnnot->contents());
      link->setName(popplerLinkAnnot->uniqueName());
      link->setLastModified(popplerLinkAnnot->modificationDate().toString(Qt::ISODate));
      // TODO: Does poppler provide the color anywhere?
      // FIXME: Convert flags

      // Note: Poppler::LinkAnnotation::HighlightMode is identical to PDFLinkAnnotation::HighlightingMode
      link->setHighlightingMode((PDFLinkAnnotation::HighlightingMode)popplerLinkAnnot->linkHighlightMode());
      // TODO: Does Poppler provide an easy interface to all quadPoints?
      break;
    }
    _links << link;
  }
  return _links;
}


// vim: set sw=2 ts=2 et

