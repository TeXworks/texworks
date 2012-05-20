#	This is part of TeXworks, an environment for working with TeX documents
#	Copyright (C) 2007-2012  Jonathan Kew, Stefan Löffler, Charlie Sharpsteen
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
#	For links to further information, or to contact the authors,
#	see <http://www.tug.org/texworks/>.

TEMPLATE	=	app
DEPENDPATH	+=	./src
INCLUDEPATH	+=	./src

SUBDIRS		+=	plugins-src/TWLuaPlugin
SUBDIRS		+=	plugins-src/TWPythonPlugin

MOC_DIR     = ./moc
# NOTE: BSD make fails if OBJECTS_DIR = ./obj (or rather if a directory obj exists at all; see issue 76)
OBJECTS_DIR = ./objs
UI_DIR      = ./ui
RCC_DIR     = ./rcc

# comment this out if poppler's xpdf headers are not available on the build system
QMAKE_CXXFLAGS += -DHAVE_POPPLER_XPDF_HEADERS

# maximum compression for resources (unless that only produces a 5% size decrease)
QMAKE_RESOURCE_FLAGS += -threshold 5 -compress 9

# avoid warnings about "#pragma mark" on non-Mac/non-XCode systems
QMAKE_CXXFLAGS_WARN_ON += -Wno-unknown-pragmas

# put all symbols in the dynamic symbol table to plugins can access them; if not
# given, plugin loading may fail with a debug warning for some plugins
# Note: only works for gnu compilers; need to check what flags to pass to other compilers
!macx {
	QMAKE_LFLAGS += -Wl,--export-dynamic
}

unix:!macx {
	TARGET	=	texworks
	isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr/local
	isEmpty(DATA_DIR):DATA_DIR = $$INSTALL_PREFIX/share
	isEmpty(TW_HELPPATH):TW_HELPPATH = $$DATA_DIR/texworks-help
	isEmpty(TW_PLUGINPATH):TW_PLUGINPATH = $$INSTALL_PREFIX/lib/texworks
	isEmpty(TW_DICPATH):TW_DICPATH = /usr/share/myspell/dicts
} else {
	TARGET	=	TeXworks
    QMAKE_CXXFLAGS += -fexceptions
    QMAKE_LFLAGS += -fexceptions
}

# packagers should override this to identify the source of the particular TeXworks build;
# avoid spaces or other chars that would need quoting on the command line
isEmpty(TW_BUILD_ID):TW_BUILD_ID = personal
QMAKE_CXXFLAGS += -DTW_BUILD_ID=$$TW_BUILD_ID

QT			+=	xml script scripttools
CONFIG		+=	rtti uitools

unix {
	system(./getDefaultBinPaths.sh):warning("Unable to determine TeX path, guessing defaults")
}

macx {
	QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
	QMAKE_MACOS_DEPLOYMENT_TARGET = 10.4
# These settings don't seem to actually work for me with Xcode 3.2.1;
# I have to change the gcc version manually in the project properties.
	QMAKE_CC = gcc-4.0
	QMAKE_CXX = g++-4.0

	INCLUDEPATH += /usr/local/include/poppler
	INCLUDEPATH += /usr/local/include/poppler/qt4
	INCLUDEPATH += /usr/local/include/hunspell

	LIBS += /usr/lib/libQtUiTools.a
	LIBS += -L/usr/local/lib
	LIBS += -lpoppler
	LIBS += -lpoppler-qt4
	LIBS += -lhunspell-1.2
	LIBS += -lgcc_eh
	LIBS += -lz
	LIBS += -framework CoreServices

	QMAKE_INFO_PLIST = TeXworks.plist

	CONFIG += x86 ppc

	ICON = TeXworks.icns
}

unix:!macx { # on Unix-ish platforms we rely on pkgconfig, and use dbus
	QT			+= dbus
	CONFIG		+= link_pkgconfig
	PKGCONFIG	+= hunspell
	PKGCONFIG	+= poppler-qt4
	PKGCONFIG	+= zlib

	# Enclose the path in \\\" (which later gets expanded to \", which in turn
	# gets expanded to " in the c++ code)
	QMAKE_CXXFLAGS += -DTW_HELPPATH=\\\"$$TW_HELPPATH\\\"
	QMAKE_CXXFLAGS += -DTW_PLUGINPATH=\\\"$$TW_PLUGINPATH\\\"
	QMAKE_CXXFLAGS += -DTW_DICPATH=\\\"$$TW_DICPATH\\\"
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

win32 { # paths here are specific to my setup
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

	# for the Windows build, we use static plugins:
	QMAKE_CXXFLAGS += -DSTATIC_SCRIPTING_PLUGINS

	LIBS += -Lplugins-src/TWLuaPlugin/release -lTWLuaPlugin
	LIBS += -llua

	LIBS += -Lplugins-src/TWPythonPlugin/release -lTWPythonPlugin
	LIBS += -Lc:/Python26/libs -lpython26
}


# Input
HEADERS	+=	src/TWApp.h \
			src/TWUtils.h \
			src/TWScriptable.h \
			src/TWScript.h \
			src/TWScriptAPI.h \
			src/TeXDocument.h \
			src/CommandlineParser.h \
			src/CompletingEdit.h \
			src/TeXHighlighter.h \
			src/TeXDocks.h \
			src/PDFDocument.h \
			src/PDFDocks.h \
			src/FindDialog.h \
			src/PrefsDialog.h \
			src/TemplateDialog.h \
			src/HardWrapDialog.h \
			src/ResourcesDialog.h \
			src/ScriptManager.h \
			src/ConfirmDelete.h \
			src/TWVersion.h \
			src/TWTextCodecs.h \
			src/SvnRev.h \
			src/synctex_parser.h \
			src/synctex_parser_utils.h \
			src/ClickableLabel.h \
			src/ConfigurableApp.h \
			src/TWSystemCmd.h

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
			src/ResourcesDialog.ui \
			src/ScriptManager.ui \
			src/ConfirmDelete.ui

SOURCES	+=	src/main.cpp \
			src/TWApp.cpp \
			src/TWUtils.cpp \
			src/TWScriptable.cpp \
			src/TWScript.cpp \
			src/TWScriptAPI.cpp \
			src/TeXDocument.cpp \
			src/CommandlineParser.cpp \
			src/CompletingEdit.cpp \
			src/TeXHighlighter.cpp \
			src/TeXDocks.cpp \
			src/PDFDocument.cpp \
			src/PDFDocks.cpp \
			src/FindDialog.cpp \
			src/PrefsDialog.cpp \
			src/TemplateDialog.cpp \
			src/TWTextCodecs.cpp \
			src/HardWrapDialog.cpp \
			src/ResourcesDialog.cpp \
			src/ScriptManager.cpp \
			src/ConfirmDelete.cpp \
			src/synctex_parser.c \
			src/synctex_parser_utils.c

RESOURCES	+=	res/resources.qrc \
				res/resfiles.qrc

TRANSLATIONS	+=	trans/TeXworks_af.ts \
					trans/TeXworks_ar.ts \
					trans/TeXworks_ca.ts \
					trans/TeXworks_cs.ts \
					trans/TeXworks_de.ts \
					trans/TeXworks_es.ts \
					trans/TeXworks_fa.ts \
					trans/TeXworks_fo.ts \
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

unix:!macx { # installation on Unix-ish platforms
	isEmpty(BIN_DIR):BIN_DIR = $$INSTALL_PREFIX/bin
	isEmpty(DOCS_DIR):DOCS_DIR = $$DATA_DIR/doc/texworks
	isEmpty(ICON_DIR):ICON_DIR = $$DATA_DIR/pixmaps
	isEmpty(MAN_DIR):MAN_DIR = $$DATA_DIR/man/man1
	isEmpty(DESKTOP_DIR):DESKTOP_DIR = $$DATA_DIR/applications

	target.path = $$BIN_DIR
	documentation.files = COPYING README NEWS
	documentation.path = $$DOCS_DIR
	manual.files = manual/*
	manual.path = $$TW_HELPPATH
	icon.files = res/images/TeXworks.png
	icon.path = $$ICON_DIR
	man.files = man/texworks.1
	man.path = $$MAN_DIR
	desktop.files = texworks.desktop
	desktop.path = $$DESKTOP_DIR
	INSTALLS = target documentation manual icon man desktop
}
