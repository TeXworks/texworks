/*
  This is part of TeXworks, an environment for working with TeX documents
  Copyright (C) 2014  Stefan Löffler, Jonathan Kew

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.

  For links to further information, or to contact the authors,
  see <http://www.tug.org/texworks/>.
*/

#include "TWSynchronizer.h"

#include <QFileInfo>
#include <QDir>

TWSyncTeXSynchronizer::TWSyncTeXSynchronizer(const QString & filename)
{
  _scanner = SyncTeX::synctex_scanner_new_with_output_file(filename.toLocal8Bit().data(), NULL, 1);
}

TWSyncTeXSynchronizer::~TWSyncTeXSynchronizer()
{
  if (_scanner != NULL)
    synctex_scanner_free(_scanner);
}

bool TWSyncTeXSynchronizer::isValid() const
{
  return (_scanner != NULL);
}


QString TWSyncTeXSynchronizer::syncTeXFilename() const
{
  if (!_scanner)
    return QString();
  return QString::fromLocal8Bit(SyncTeX::synctex_scanner_get_synctex(_scanner));
}

QString TWSyncTeXSynchronizer::pdfFilename() const
{
  if (!_scanner)
    return QString();
  return QString::fromLocal8Bit(SyncTeX::synctex_scanner_get_output(_scanner));
}

//virtual
TWSynchronizer::PDFSyncPoint TWSyncTeXSynchronizer::syncFromTeX(const TWSynchronizer::TeXSyncPoint & src) const
{
  PDFSyncPoint retVal;

  // Find the name SyncTex is using for this source file...
  const QFileInfo sourceFileInfo(src.filename);
  QDir curDir(QFileInfo(pdfFilename()).canonicalPath());
  SyncTeX::synctex_node_t node = SyncTeX::synctex_scanner_input(_scanner);
  QString name;
  bool found = false;
  while (node) {
    name = QString::fromLocal8Bit(SyncTeX::synctex_scanner_get_name(_scanner, SyncTeX::synctex_node_tag(node)));
    const QFileInfo fi(curDir, name);
    if (fi == sourceFileInfo) {
      found = true;
      break;
    }
    node = synctex_node_sibling(node);
  }
  if (!found)
    return retVal;

  retVal.filename = pdfFilename();

  if (SyncTeX::synctex_display_query(_scanner, name.toLocal8Bit().data(), src.line, src.col) > 0) {
    retVal.page = -1;
    while ((node = SyncTeX::synctex_next_result(_scanner)) != NULL) {
      if (retVal.page < 0)
        retVal.page = SyncTeX::synctex_node_page(node);
      if (SyncTeX::synctex_node_page(node) != retVal.page)
        continue;
      QRectF nodeRect(synctex_node_box_visible_h(node),
                      synctex_node_box_visible_v(node) - synctex_node_box_visible_height(node),
                      synctex_node_box_visible_width(node),
                      synctex_node_box_visible_height(node) + synctex_node_box_visible_depth(node));
      retVal.rects.append(nodeRect);
    }
  }

  return retVal;
}

//virtual
TWSynchronizer::TeXSyncPoint TWSyncTeXSynchronizer::syncFromPDF(const TWSynchronizer::PDFSyncPoint & src) const
{
  TeXSyncPoint retVal;

  if (src.rects.length() != 1)
    return retVal;

  if (SyncTeX::synctex_edit_query(_scanner, src.page, src.rects[0].left(), src.rects[0].top()) > 0) {
    SyncTeX::synctex_node_t node;
    while ((node = SyncTeX::synctex_next_result(_scanner)) != NULL) {
      retVal.filename = QString::fromLocal8Bit(SyncTeX::synctex_scanner_get_name(_scanner, SyncTeX::synctex_node_tag(node)));
      retVal.line = SyncTeX::synctex_node_line(node);
      retVal.col = -1;
      break; // FIXME: currently we just take the first hit
    }
  }

  return retVal;
}
