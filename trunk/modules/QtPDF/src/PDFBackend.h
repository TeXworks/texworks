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
#ifndef PDFBackend_H
#define PDFBackend_H

#include <PDFAnnotations.h>

#include <QImage>
#include <QFileInfo>
#include <QSharedPointer>
#include <QThread>
#include <QStack>
#include <QCache>
#include <QMutex>
#include <QReadWriteLock>
#include <QWaitCondition>
#include <QEvent>


// Backend Rendering
// =================

class Page;
class Document;

// TODO: Find a better place to put this
QDateTime fromPDFDate(QString pdfDate);

class PDFFontDescriptor
{
public:
  enum FontStretch { FontStretch_UltraCondensed, FontStretch_ExtraCondensed, \
                     FontStretch_Condensed, FontStretch_SemiCondensed, \
                     FontStretch_Normal, FontStretch_SemiExpanded, \
                     FontStretch_Expanded, FontStretch_ExtraExpanded, \
                     FontStretch_UltraExpanded };
  enum Flag { Flag_FixedPitch = 0x01, Flag_Serif = 0x02, Flag_Symbolic = 0x04, \
              Flag_Script = 0x08, Flag_Nonsymbolic = 0x20, Flag_Italic = 0x40, \
              Flag_AllCap = 0x10000, Flag_SmallCap = 0x20000, \
              Flag_ForceBold = 0x40000 };
  Q_DECLARE_FLAGS(Flags, Flag)

  PDFFontDescriptor(const QString fontName = QString());
  virtual ~PDFFontDescriptor() { }

  bool isSubset() const;

  QString name() const { return _name; }
  // pureName() removes the subset tag
  QString pureName() const;

  void setName(const QString name) { _name = name; }
  // TODO: Accessor methods for all other properties

protected:
  // From pdf specs
  QString _name;
  QString _family;
  enum FontStretch _stretch;
  int _weight;
  Flags _flags;
  QRectF _bbox;
  float _italicAngle;
  float _ascent;
  float _descent;
  float _leading;
  float _capHeight;
  float _xHeight;
  float _stemV;
  float _stemH;
  float _avgWidth;
  float _maxWidth;
  float _missingWidth;
  QString _charSet;

  // From pdf specs for CID fonts only
  // _style
  // _lang
  // _fD
  // _CIDSet
};

// Note: This is a hack, but since all the information (with the exception of
// the type of font) we use (and that is provided by poppler) is encapsulated in
// PDFFontDescriptor, there is no use right now to completely implement all the
// different font structures
class PDFFontInfo
{
public:
  enum FontType { FontType_Type0, FontType_Type1, FontType_MMType1, \
                  FontType_Type3, FontType_TrueType };
  enum CIDFontType { CIDFont_None, CIDFont_Type0, CIDFont_Type2 };
  enum FontProgramType { ProgramType_None, ProgramType_Type1, \
                         ProgramType_TrueType, ProgramType_Type1CFF, \
                         ProgramType_CIDCFF, ProgramType_OpenType };
  enum FontSource { Source_Embedded, Source_File, Source_Builtin };
  
  PDFFontInfo() { };
  virtual ~PDFFontInfo() { };
  
  FontType fontType() const { return _fontType; }
  CIDFontType CIDType() const { return _CIDType; }
  FontProgramType fontProgramType() const { return _fontProgramType; }
  PDFFontDescriptor descriptor() const { return _descriptor; }
  // returns the path to the file used for rendering this font, or an invalid
  // QFileInfo for embedded fonts
  QFileInfo fileName() const { return _substitutionFile; }

  bool isSubset() const { return _descriptor.isSubset(); }
  FontSource source() const { return _source; }

  // TODO: Implement some advanced logic; e.g., non-embedded fonts have no font
  // program type
  void setFontType(const FontType fontType) { _fontType = fontType; }
  void setCIDType(const CIDFontType CIDType) { _CIDType = CIDType; }
  void setFontProgramType(const FontProgramType programType) { _fontProgramType = programType; }
  void setDescriptor(const PDFFontDescriptor descriptor) { _descriptor = descriptor; }
  void setFileName(const QFileInfo file) { _source = Source_File; _substitutionFile = file; }
  void setSource(const FontSource source) { _source = source; }

protected:
  FontSource _source;
  PDFFontDescriptor _descriptor;
  QFileInfo _substitutionFile;
  FontType _fontType;
  CIDFontType _CIDType;
  FontProgramType _fontProgramType;
};


class PDFPageTile
{

public:
  // TODO:
  // We may want an application-wide cache instead of a document-specific cache
  // to keep memory usage down. This may require an additional piece of
  // information---the document that the page belongs to.
  PDFPageTile(double xres, double yres, QRect render_box, int page_num):
    xres(xres), yres(yres),
    render_box(render_box),
    page_num(page_num)
  {}

  double xres, yres;
  QRect render_box;
  int page_num;

  bool operator==(const PDFPageTile &other) const
  {
    return (xres == other.xres && yres == other.yres && render_box == other.render_box && page_num == other.page_num);
  }

};
// Need a hash function in order to allow `PDFPageTile` to be used as a key
// object for a `QCache`.
uint qHash(const PDFPageTile &tile);

// This class is thread-safe
class PDFPageCache : protected QCache<PDFPageTile, QSharedPointer<QImage> >
{
public:
  PDFPageCache() { }
  virtual ~PDFPageCache() { }

  // Note: Each image has a cost of 1
  int maxSize() const { return maxCost(); }
  void setMaxSize(const int num) { setMaxCost(num); }

  // Returns the image under the key `tile` or NULL if it doesn't exist
  QSharedPointer<QImage> getImage(const PDFPageTile & tile) const;
  // Returns the pointer to the image in the cache under they key `tile` after
  // the insertion. If overwrite == true, this will always be image, otherwise
  // it can be different
  QSharedPointer<QImage> setImage(const PDFPageTile & tile, QImage * image, const bool overwrite = true);
  
  void lock() const { _lock.lockForRead(); }
  void unlock() const { _lock.unlock(); }

  QList<PDFPageTile> tiles() const { return keys(); }
protected:
  mutable QReadWriteLock _lock;
};

class PageProcessingRequest : public QObject
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRequest(Page *page, QObject *listener) : page(page), listener(listener) { }
  // Should perform whatever processing it is designed to do
  // Returns true if finished successfully, false otherwise
  virtual bool execute() = 0;

public:
  enum Type { PageRendering, LoadLinks };

  virtual ~PageProcessingRequest() { }
  virtual Type type() const = 0;

  Page *page;
  QObject *listener;
  
  virtual bool operator==(const PageProcessingRequest & r) const;
};


class PageProcessingRenderPageRequest : public PageProcessingRequest
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

public:
  PageProcessingRenderPageRequest(Page *page, QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false) :
    PageProcessingRequest(page, listener),
    xres(xres), yres(yres),
    render_box(render_box),
    cache(cache)
  {}
  Type type() const { return PageRendering; }

  virtual bool operator==(const PageProcessingRequest & r) const;

protected:
  bool execute();

  double xres, yres;
  QRect render_box;
  bool cache;
};


class PDFPageRenderedEvent : public QEvent
{

public:
  PDFPageRenderedEvent(double xres, double yres, QRect render_rect, QImage rendered_page):
    QEvent(PageRenderedEvent),
    xres(xres), yres(yres),
    render_rect(render_rect),
    rendered_page(rendered_page)
  {}

  static const QEvent::Type PageRenderedEvent;

  const double xres, yres;
  const QRect render_rect;
  const QImage rendered_page;

};


class PageProcessingLoadLinksRequest : public PageProcessingRequest
{
  Q_OBJECT
  friend class PDFPageProcessingThread;

public:
  PageProcessingLoadLinksRequest(Page *page, QObject *listener) : PageProcessingRequest(page, listener) { }
  Type type() const { return LoadLinks; }

protected:
  bool execute();
};


class PDFLinksLoadedEvent : public QEvent
{

public:
  PDFLinksLoadedEvent(const QList< QSharedPointer<PDFLinkAnnotation> > links):
    QEvent(LinksLoadedEvent),
    links(links)
  {}

  static const QEvent::Type LinksLoadedEvent;

  const QList< QSharedPointer<PDFLinkAnnotation> > links;

};


// Class to perform (possibly) lengthy operations on pages in the background
// Modelled after the "Blocking Fortune Client Example" in the Qt docs
// (http://doc.qt.nokia.com/stable/network-blockingfortuneclient.html)
class PDFPageProcessingThread : public QThread
{
  Q_OBJECT

public:
  PDFPageProcessingThread();
  virtual ~PDFPageProcessingThread();

  // add a processing request to the work stack
  // Note: request must have been created on the heap and must be in the scope
  // of this thread; use requestRenderPage() and requestLoadLinks() for that
  void addPageProcessingRequest(PageProcessingRequest * request);

protected:
  virtual void run();

private:
  QStack<PageProcessingRequest*> _workStack;
  QMutex _mutex;
  QWaitCondition _waitCondition;
  bool _quit;
#ifdef DEBUG
  QTime _renderTimer;
#endif

};

class PDFToCItem
{
public:
  enum PDFToCItemFlag { Flag_Italic = 0x1, Flag_Bold = 0x2 };
  Q_DECLARE_FLAGS(PDFToCItemFlags, PDFToCItemFlag)

  PDFToCItem(const QString label = QString()) : _label(label), _isOpen(false), _action(NULL) { }
  PDFToCItem(const PDFToCItem & o) : _label(o._label), _isOpen(o._isOpen), _color(o._color), _children(o._children), _flags(o._flags) {
    _action = (o._action ? o._action->clone() : NULL);
  }
  virtual ~PDFToCItem() { if (_action) delete _action; }

  QString label() const { return _label; }
  bool isOpen() const { return _isOpen; }
  PDFAction * action() const { return _action; }
  QColor color() const { return _color; }
  const QList<PDFToCItem> & children() const { return _children; }
  QList<PDFToCItem> & children() { return _children; }
  PDFToCItemFlags flags() const { return _flags; }
  PDFToCItemFlags & flags() { return _flags; }
  
  void setLabel(const QString label) { _label = label; }
  void setOpen(const bool isOpen = true) { _isOpen = isOpen; }
  void setAction(PDFAction * action) {
    if (_action)
      delete _action;
    _action = action;
  }
  void setColor(const QColor color) { _color = color; }

protected:
  QString _label;
  bool _isOpen; // derived from the sign of the `Count` member of the outline item dictionary
  PDFAction * _action; // if the `Dest` member of the outline item dictionary is set, it must be converted to a PDFGotoAction
  QColor _color;
  QList<PDFToCItem> _children;
  PDFToCItemFlags _flags;
};

typedef QList<PDFToCItem> PDFToC;

struct  SearchRequest
{
  QSharedPointer<Document> doc;
  int pageNum;
  QString searchString;
};

struct  SearchResult
{
  int pageNum;
  QRectF bbox;
};


// PDF ABCs
// ========
// This header file defines a set of Abstract Base Classes (ABCs) for PDF
// documents. Having a set of abstract classes allows tools like GUI viewers to
// be written that are agnostic to the library that provides the actual PDF
// implementation: Poppler, MuPDF, etc.
// TODO: Should this class be derived from QObject to emit signals (e.g., 
// documentChanged() after reload, unlocking, etc.)?

class Document
{
  friend class Page;

public:
  enum TrappedState { Trapped_Unknown, Trapped_True, Trapped_False };
  enum Permission { Permission_Print = 0x0004,
                    Permission_Change = 0x0008,
                    Permission_Extract = 0x0010, // text and graphics
                    Permission_Annotate = 0x0020, // Also includes filling forms
                    Permission_FillForm = 0x0100,
                    Permission_ExtractForAccessibility = 0x0200,
                    Permission_Assemble = 0x0400,
                    Permission_PrintHighRes = 0x0800
                  };
  Q_DECLARE_FLAGS(Permissions, Permission)

  Document(QString fileName);
  virtual ~Document();

  int numPages();
  PDFPageProcessingThread& processingThread();
  PDFPageCache& pageCache();

  virtual QSharedPointer<Page> page(int at)=0;
  virtual PDFDestination resolveDestination(const PDFDestination & namedDestination) const {
    return (namedDestination.isExplicit() ? namedDestination : PDFDestination());
  }

  QFlags<Permissions> permissions() const { return _permissions; }
  QFlags<Permissions>& permissions() { return _permissions; }

  virtual bool isValid() const = 0;
  virtual bool isLocked() const = 0;

  // Returns `true` if unlocking was successful and `false` otherwise.  
  virtual bool unlock(const QString password) = 0;

  // Override in derived class if it provides access to the document outline
  // strutures of the pdf file.
  virtual PDFToC toc() const { return PDFToC(); }
  virtual QList<PDFFontInfo> fonts() const { return QList<PDFFontInfo>(); }

  // <metadata>
  QString title() const { return _meta_title; }
  QString author() const { return _meta_author; }
  QString subject() const { return _meta_subject; }
  QString keywords() const { return _meta_keywords; }
  QString creator() const { return _meta_creator; }
  QString producer() const { return _meta_producer; }
  QDateTime creationDate() const { return _meta_creationDate; }
  QDateTime modDate() const { return _meta_modDate; }
  TrappedState trapped() const { return _meta_trapped; }
  QMap<QString, QString> metaDataOther() const { return _meta_other; }
  // </metadata>

  // Searches the entire document for the given string and returns a list of
  // boxes that contain that text.
  //
  // TODO:
  //
  //   - Implement as a function that returns a generator object which can
  //     return the search results one at a time rather than all at once.
  //
  //   - See TODO list in `Page::search`
  virtual QList<SearchResult> search(QString searchText, int startPage=0);

protected:
  int _numPages;
  PDFPageProcessingThread _processingThread;
  PDFPageCache _pageCache;
  QVector< QSharedPointer<Page> > _pages;
  QFlags<Permissions> _permissions;

  QString _fileName;

  QString _meta_title;
  QString _meta_author;
  QString _meta_subject;
  QString _meta_keywords;
  QString _meta_creator;
  QString _meta_producer;
  QDateTime _meta_creationDate;
  QDateTime _meta_modDate;
  TrappedState _meta_trapped;
  QMap<QString, QString> _meta_other;
};

class Page
{

protected:
  Document *_parent;
  const int _n;

public:
  Page(Document *parent, int at);
  virtual ~Page();

  int pageNum();
  virtual QSizeF pageSizeF() const = 0;

  Document * document() { return _parent; }

  virtual QImage renderToImage(double xres, double yres, QRect render_box = QRect(), bool cache = false)=0;
  virtual void asyncRenderToImage(QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false);

  virtual QList< QSharedPointer<PDFLinkAnnotation> > loadLinks() = 0;
  virtual void asyncLoadLinks(QObject *listener);

  QSharedPointer<QImage> getCachedImage(double xres, double yres, QRect render_box = QRect());
  // Returns either a cached image (if it exists), or triggers a render request.
  // If listener != NULL, this is an asynchronous render request and the method
  // returns a dummy image (which is added to the cache to speed up future
  // requests). Otherwise, the method renders the page synchronously and returns
  // the result.
  QSharedPointer<QImage> getTileImage(QObject * listener, const double xres, const double yres, QRect render_box = QRect());


  // Searches the page for the given text string and returns a list of boxes
  // that contain that text.
  //
  // TODO:
  //
  // Implement as a function that returns a generator object which can return
  // the search results one at a time rather than all at once which is time
  // consuming. Even better, allow the returned object to be used as a C++
  // iterator---then we could pass it off to QtConcurrent to generate results
  // in the background and access them through a QFuture.
  //
  // This is very tricky to do in C++. God I miss Python and its `itertools`
  // library.
  virtual QList<SearchResult> search(QString searchText) = 0;

  static QList<SearchResult> search(SearchRequest request);
};


// Backend Implementations
// =======================
// These provide library-specific concrete impelemntations of the abstract base
// classes defined here.
#ifdef USE_POPPLER
#include <backends/PopplerBackend.h> // Invokes GPL v2+ License
#endif
#ifdef USE_MUPDF
#include <backends/MuPDFBackend.h>   // Invokes GPL v3 License
#endif

#endif // End header guard
// vim: set sw=2 ts=2 et

