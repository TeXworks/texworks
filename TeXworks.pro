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
#	see <http://texworks.org/>.

TEMPLATE	=	app
DEPENDPATH	+=	./src
INCLUDEPATH	+=	./src

MOC_DIR     = ./moc
OBJECTS_DIR = ./obj
UI_DIR      = ./ui
RCC_DIR     = ./rcc

unix:!macx {
	TARGET	=	texworks
} else {
	TARGET	=	TeXworks
}

QT			+=	xml

macx {
	INCLUDEPATH += /usr/local/include/poppler
	INCLUDEPATH += /usr/local/include/poppler/qt4
	INCLUDEPATH += /usr/local/include/hunspell

	LIBS += -L/usr/local/lib
	LIBS += -lpoppler-qt4
	LIBS += -lhunspell-1.2

	QMAKE_INFO_PLIST = TeXworks.plist

	CONFIG += x86 ppc

	ICON = TeXworks.icns
}

unix:!macx { # on Unix-ish platforms we rely on pkgconfig, and use dbus
	QT			+= dbus
	CONFIG		+= link_pkgconfig
	PKGCONFIG	+= hunspell
	PKGCONFIG	+= poppler-qt4
	PKGCONFIG	+= dbus-1
}

linux-g++ {
	# Qt/dbus config on Debian is broken, hence the lines below
	LIBS		+= -lQtDBus
	INCLUDEPATH	+= /usr/include/qt4/QtDBus
}

openbsd-g++ {
	# Same bug exists in OpenBSD/qt4
	LIBS		+= -lQtDBus
	INCLUDEPATH	+= /usr/local/include/X11/qt4/QtDBus
}

win32 { # paths here are specific to my cross-compilation setup
	INCLUDEPATH += z:/cross-tools/usr/local/include
	INCLUDEPATH += z:/cross-tools/usr/local/include/poppler
	INCLUDEPATH += z:/cross-tools/usr/local/include/poppler/qt4
	INCLUDEPATH += z:/cross-tools/usr/local/include/hunspell

	LIBS += -Lz:/cross-tools/usr/local/lib
	LIBS += -lpoppler-qt4
	LIBS += -lpoppler
	LIBS += -lfreetype
	LIBS += -lhunspell-1.2
	LIBS += -lz
}

# Input
HEADERS	+=	src/TWApp.h \
			src/TWUtils.h \
			src/TeXDocument.h \
			src/CompletingEdit.h \
			src/TeXHighlighter.h \
			src/TeXDocks.h \
			src/PDFDocument.h \
			src/PDFDocks.h \
			src/FindDialog.h \
			src/PrefsDialog.h \
			src/TemplateDialog.h \
			src/ConfirmDelete.h \
			src/synctex_parser.h

FORMS	+=	src/TeXDocument.ui \
			src/PDFDocument.ui \
			src/Find.ui \
			src/PDFFind.ui \
			src/Replace.ui \
			src/SearchResults.ui \
			src/PrefsDialog.ui \
			src/ToolConfig.ui \
			src/TemplateDialog.ui \
			src/ConfirmDelete.ui

SOURCES	+=	src/main.cpp \
			src/TWApp.cpp \
			src/TWUtils.cpp \
			src/TeXDocument.cpp \
			src/CompletingEdit.cpp \
			src/TeXHighlighter.cpp \
			src/TeXDocks.cpp \
			src/PDFDocument.cpp \
			src/PDFDocks.cpp \
			src/FindDialog.cpp \
			src/PrefsDialog.cpp \
			src/TemplateDialog.cpp \
			src/ConfirmDelete.cpp \
			src/synctex_parser.c

RESOURCES	+=	res/resources.qrc \
				res/resfiles.qrc

TRANSLATIONS	+=	trans/TeXworks_ar.ts \
					trans/TeXworks_de.ts \
					trans/TeXworks_fr.ts \
					trans/TeXworks_it.ts \
					trans/TeXworks_nl.ts \
					trans/TeXworks_ru.ts \
					trans/TeXworks_tr.ts
