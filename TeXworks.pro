#	This is part of TeXworks, an environment for working with TeX documents
#	Copyright (C) 2007-08  Jonathan Kew
#
#	This program is free software; you can redistribute it and/or modify
#	it under the terms of the GNU General Public License as published by
#	the Free Software Foundation; either version 2 of the License, or
#	(at your option) any later version.
#
#	This program is distributed in the hope that it will be useful,
#	but WITHOUT ANY WARRANTY; without even the implied warranty of
#	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#	GNU General Public License for more details.
#
#	You should have received a copy of the GNU General Public License
#	along with this program.  If not, see <http://www.gnu.org/licenses/>.
#
#	For links to further information, or to contact the author,
#	see <http://tug.org/texworks/>.

TEMPLATE	=	app
DEPENDPATH	+=	.
INCLUDEPATH	+=	.

linux-g++ {
	TARGET		=	texworks
} else {
	TARGET		=	TeXworks
}

QT			+=	xml

macx {
	INCLUDEPATH += /usr/local/include/poppler
	INCLUDEPATH += /usr/local/include/poppler/qt4

	LIBS += -L/usr/local/lib
	LIBS += -lpoppler-qt4
	LIBS += -lhunspell-1.2

	QMAKE_INFO_PLIST = TeXworks.plist

	ICON = TeXworks.icns
}

linux-g++ {
	INCLUDEPATH += /usr/include/poppler
	INCLUDEPATH += /usr/include/poppler/qt4

	LIBS += -lpoppler-qt4
	LIBS += -lhunspell
}

win32 {
	INCLUDEPATH += X:/QTeX-libs/poppler-mingw32/poppler/inst/usr/local/include/poppler
	INCLUDEPATH += X:/QTeX-libs/poppler-mingw32/poppler/inst/usr/local/include/poppler/qt4

	LIBS += -LX:/QTeX-libs/poppler-mingw32/poppler/inst/usr/local/lib
	LIBS += -L"C:/Program Files/GnuWin32/lib"
	LIBS += -lpoppler-qt4
	LIBS += -lpoppler
	LIBS += -lfreetype
	LIBS += -lhunspell-1.2
}

# Input
HEADERS	+=	TWApp.h \
			TWUtils.h \
			TeXDocument.h \
			CompletingEdit.h \
			TeXHighlighter.h \
			PDFDocument.h \
			PDFDocks.h \
			FindDialog.h \
			PrefsDialog.h \
			TemplateDialog.h \
			synctex_parser.h

FORMS	+=	TeXDocument.ui \
			PDFDocument.ui \
			Find.ui \
			Replace.ui \
			PrefsDialog.ui \
			ToolConfig.ui \
			TemplateDialog.ui

SOURCES	+=	main.cpp \
			TWApp.cpp \
			TWUtils.cpp \
			TeXDocument.cpp \
			CompletingEdit.cpp \
			TeXHighlighter.cpp \
			PDFDocument.cpp \
			PDFDocks.cpp \
			FindDialog.cpp \
			PrefsDialog.cpp \
			TemplateDialog.cpp \
			synctex_parser.c

RESOURCES	+=	resources.qrc \
				resfiles.qrc

