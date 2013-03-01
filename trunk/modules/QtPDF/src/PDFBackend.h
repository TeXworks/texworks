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

// FIXME: Thin the header inclusion down.
#include <QtCore>
#include <QImage>
#include <QApplication>
#include <QFlags>
#include <QColor>


// Backend Rendering
// =================

class Page;
class Document;

// TODO: Find a better place to put this
QDateTime fromPDFDate(QString pdfDate);


// FIXME: Annotations and Actions should probably be moved to separate files

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
  // TODO: _flags, _appearance, _appearanceState, _border, _structParent, _optContent
  QFlags<AnnotationFlags> _flags;
  // QList<???> _appearance;
  // ??? _appearanceState;
  // ??? _border;
  QColor _color;
  // ??? _structParent;
  // ??? _optContent;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(PDFAnnotation::AnnotationFlags)

class PDFDestination
{
public:
  enum Type { Destination_XYZ, Destination_Fit, Destination_FitH, \
              Destination_FitV, Destination_FitR, Destination_FitB, \
              Destination_FitBH, Destination_FitBV };
  PDFDestination(const int page = -1) : _page(page), _type(Destination_XYZ), _rect(QRectF(-1, -1, -1, -1)), _zoom(-1) { }
  PDFDestination(const QString destinationName) : _destinationName(destinationName) { }

  bool isValid() const { return _page >= 0 || !_destinationName.isEmpty(); }
  // If the destination is not explicit (i.e., it is a named destination), use
  // Document::resolveDestination() to resolve (this must be done just-in-time
  // in case it refers to another document, or the name-to-destination mapping
  // has changed since the PDFDestination object was constructed.
  bool isExplicit() const { return _destinationName.isEmpty() && _page >= 0; }

  int page() const { return _page; }
  Type type() const { return _type; }
  QString destinationName() const { return _destinationName; }
  float zoom() const { return _zoom; }
  float top() const { return _rect.top(); }
  float left() const { return _rect.left(); }
  QRectF rect() const { return _rect; }

  // Returns the new viewport in the new page's coordinate system
  // Note: the returned viewport may have a different aspect ratio than
  // oldViewport. In that case, it view should be centered around the returned
  // rect.
  // Params:
  //  - oldViewport: viewport in old page's coordinate system
  //  - oldZoom
  QRectF viewport(const Document * doc, const QRectF oldViewport, const float oldZoom) const;
  
  void setPage(const int page) { _page = page; }
  void setType(const Type type) { _type = type; }
  void setZoom(const float zoom) { _zoom = zoom; }
  void setRect(const QRectF rect) { _rect = rect; }
  void setDestinationName(const QString destinationName) { _destinationName = destinationName; }

private:
  int _page;
  Type _type;
  QString _destinationName;
  QRectF _rect; // depending on _type, only some of the components might be significant
  float _zoom;
};

#ifdef DEBUG
  QDebug operator<<(QDebug dbg, const PDFDestination & dest);
#endif


// TODO: Possibly merge ActionTypeGoTo, ActionTypeGoToR, ActionTypeGoToE
class PDFAction
{
public:
  enum ActionType {
    ActionTypeGoTo, /*ActionTypeGoToR,*/ ActionTypeGoToE, ActionTypeLaunch,
    ActionTypeThread, ActionTypeURI, ActionTypeSound, ActionTypeMovie,
    ActionTypeHide, ActionTypeNamed, ActionTypeSubmitForm, ActionTypeResetForm,
    ActionTypeImportData, ActionTypeJavaScript, ActionTypeSetOCGState,
    ActionTypeRendition, ActionTypeTrans, ActionTypeGoTo3DView
  };

  virtual ActionType type() const = 0;
  virtual PDFAction * clone() const = 0;
};

class PDFURIAction : public PDFAction
{
public:
  PDFURIAction(const QUrl url) : _url(url), _isMap(false) { }
  PDFURIAction(const PDFURIAction & a) : _url(a._url), _isMap(a._isMap) { }
  
  ActionType type() const { return ActionTypeURI; }
  PDFAction * clone() const { return new PDFURIAction(*this); }

  // TODO: handle _isMap (see PDF 1.7 specs)
  QUrl url() const { return _url; }

private:
  QUrl _url;
  bool _isMap;
};

class PDFGotoAction : public PDFAction
{
public:
  PDFGotoAction(const PDFDestination destination = PDFDestination()) : _destination(destination), _isRemote(false), _openInNewWindow(false) { }
  PDFGotoAction(const PDFGotoAction & a) : _destination(a._destination), _isRemote(a._isRemote), _filename(a._filename), _openInNewWindow(a._openInNewWindow) { }

  ActionType type() const { return ActionTypeGoTo; }
  PDFAction * clone() const { return new PDFGotoAction(*this); }

  PDFDestination destination() const { return _destination; }
  bool isRemote() const { return _isRemote; }
  QString filename() const { return _filename; }
  bool openInNewWindow() const { return _openInNewWindow; }

  void setDestination(const PDFDestination destination) { _destination = destination; }
  void setRemote(const bool remote = true) { _isRemote = remote; }
  void setFilename(const QString filename) { _filename = filename; }
  void setOpenInNewWindow(const bool openInNewWindow = true) { _openInNewWindow = openInNewWindow; }

private:
  PDFDestination _destination;
  bool _isRemote;
  QString _filename; // relevent only if _isRemote == true; should always refer to a PDF document (for other files, use PDFLaunchAction)
  bool _openInNewWindow; // relevent only if _isRemote == true
};

class PDFLaunchAction : public PDFAction
{
public:
  PDFLaunchAction(const QString command) : _command(command) { }

  ActionType type() const { return ActionTypeLaunch; }
  PDFAction * clone() const { return new PDFLaunchAction(*this); }
  
  QString command() const { return _command; }
  void setCommand(const QString command) { _command = command; }

  // FIXME: handle newWindow, implement OS-specific extensions
private:
  QString _command;
  bool _newWindow;
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

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingRenderPageRequest(Page *page, QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false) :
    PageProcessingRequest(page, listener),
    xres(xres), yres(yres),
    render_box(render_box),
    cache(cache)
  {}

  bool execute();

public:
  Type type() const { return PageRendering; }
  
  virtual bool operator==(const PageProcessingRequest & r) const;

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

  // Protect c'tor and execute() so we can't access them except in derived
  // classes and friends
protected:
  PageProcessingLoadLinksRequest(Page *page, QObject *listener) : PageProcessingRequest(page, listener) { }
  bool execute();

public:
  Type type() const { return LoadLinks; }

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

  void requestRenderPage(Page *page, QObject *listener, double xres, double yres, QRect render_box = QRect(), bool cache = false);
  void requestLoadLinks(Page *page, QObject *listener);

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

class Document
{
  friend class Page;

public:
  enum TrappedState { Trapped_Unknown, Trapped_True, Trapped_False };

  Document(QString fileName);
  virtual ~Document();

  int numPages();
  PDFPageProcessingThread& processingThread();
  PDFPageCache& pageCache();

  virtual QSharedPointer<Page> page(int at)=0;
  virtual PDFDestination resolveDestination(const PDFDestination & namedDestination) const {
    return (namedDestination.isExplicit() ? namedDestination : PDFDestination());
  }

  virtual bool isValid() const = 0;

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
  virtual QSizeF pageSizeF()=0;

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

