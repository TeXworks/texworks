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

MOC_DIR     = ./moc
OBJECTS_DIR = ./obj
UI_DIR      = ./ui

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
HEADERS	+=	src/TWApp.h \
			src/TWUtils.h \
			src/TeXDocument.h \
			src/CompletingEdit.h \
			src/TeXHighlighter.h \
			src/PDFDocument.h \
			src/PDFDocks.h \
			src/FindDialog.h \
			src/PrefsDialog.h \
			src/TemplateDialog.h \
			src/synctex_parser.h

FORMS	+=	src/TeXDocument.ui \
			src/PDFDocument.ui \
			src/Find.ui \
			src/Replace.ui \
			src/PrefsDialog.ui \
			src/ToolConfig.ui \
			src/TemplateDialog.ui

SOURCES	+=	src/main.cpp \
			src/TWApp.cpp \
			src/TWUtils.cpp \
			src/TeXDocument.cpp \
			src/CompletingEdit.cpp \
			src/TeXHighlighter.cpp \
			src/PDFDocument.cpp \
			src/PDFDocks.cpp \
			src/FindDialog.cpp \
			src/PrefsDialog.cpp \
			src/TemplateDialog.cpp \
			src/synctex_parser.c

RESOURCES	+=	res/resources.qrc \
				res/resfiles.qrc

