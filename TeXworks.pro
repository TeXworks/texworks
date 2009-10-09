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

# comment this out if poppler's xpdf headers are not available on the build system
QMAKE_CXXFLAGS += -DHAVE_POPPLER_XPDF_HEADERS

unix:!macx {
	TARGET	=	texworks
} else {
	TARGET	=	TeXworks
}

QT			+=	xml
CONFIG		+=	rtti

unix {
	system(./getDefaultBinPaths.sh):warning("Unable to determine TeX path, guessing defaults")
}

macx {
	INCLUDEPATH += /usr/local/include/poppler
	INCLUDEPATH += /usr/local/include/poppler/qt4
	INCLUDEPATH += /usr/local/include/hunspell

	LIBS += -L/usr/local/lib
	LIBS += -lpoppler
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

	# Enclose the path in \\\" (which later gets expanded to \", which in turn
	# gets expanded to " in the c++ code)
	QMAKE_CXXFLAGS += -DTW_HELPPATH=\\\"/usr/local/share/texworks-help\\\"
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

win32 {
	QTPLUGIN += qjpeg

	# paths here are specific to my setup
	INCLUDEPATH += c:/MinGW514/local/include
	INCLUDEPATH += c:/MinGW514/local/include/poppler
	INCLUDEPATH += c:/MinGW514/local/include/poppler/qt4
	INCLUDEPATH += c:/MinGW514/local/include/hunspell

	LIBS += -Lc:/MinGW514/local/lib
	LIBS += -lpoppler-qt4
	LIBS += -lpoppler
	LIBS += -lfreetype
	LIBS += -lhunspell-1.2
	LIBS += -lz
	LIBS += -lgdi32

	RC_FILE = res/TeXworks.rc
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
			src/HardWrapDialog.h \
			src/ConfirmDelete.h \
			src/TWVersion.h \
			src/SvnRev.h \
			src/synctex_parser.h \
			src/synctex_parser_utils.h

FORMS	+=	src/TeXDocument.ui \
			src/PDFDocument.ui \
			src/Find.ui \
			src/PDFFind.ui \
			src/Replace.ui \
			src/SearchResults.ui \
			src/PrefsDialog.ui \
			src/ToolConfig.ui \
			src/TemplateDialog.ui \
			src/HardWrapDialog.ui \
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
			src/HardWrapDialog.cpp \
			src/ConfirmDelete.cpp \
			src/synctex_parser.c \
			src/synctex_parser_utils.c

RESOURCES	+=	res/resources.qrc \
				res/resfiles.qrc

TRANSLATIONS	+=	trans/TeXworks_ar.ts \
					trans/TeXworks_ca.ts \
					trans/TeXworks_cs.ts \
					trans/TeXworks_de.ts \
					trans/TeXworks_es.ts \
					trans/TeXworks_fa.ts \
					trans/TeXworks_fr.ts \
					trans/TeXworks_it.ts \
					trans/TeXworks_ja.ts \
					trans/TeXworks_ko.ts \
					trans/TeXworks_nl.ts \
					trans/TeXworks_pl.ts \
					trans/TeXworks_pt_BR.ts \
					trans/TeXworks_ru.ts \
					trans/TeXworks_sl.ts \
					trans/TeXworks_tr.ts \
					trans/TeXworks_zh_CN.ts

