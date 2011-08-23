#	This is part of TeXworks, an environment for working with TeX documents
#	Copyright (C) 2007-2011  Jonathan Kew, Stefan Löffler
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
#	see <http://www.tug.org/texworks/>.

TEMPLATE	=	lib
CONFIG		+=	plugin

# Path to the texworks base folder
INCLUDEPATH	+=	../../src/

MOC_DIR     = ./moc
OBJECTS_DIR = ./obj

TARGET	=	TWPythonPlugin

CONFIG		+=	rtti uitools

# Some code in Python breaks strict aliasing rules
# This flag tells the compiler not to assume it in the first place, thereby
# avoiding warnings and possible future errors
QMAKE_CXXFLAGS	+= -fno-strict-aliasing

macx {
	CONFIG	+= x86 ppc

	QMAKE_MAC_SDK = /Developer/SDKs/MacOSX10.4u.sdk
	QMAKE_MACOS_DEPLOYMENT_TARGET = 10.4
# These settings don't seem to actually work for me with Xcode 3.2.1;
# I have to change the gcc version manually in the project properties.
	QMAKE_CC = gcc-4.0
	QMAKE_CXX = g++-4.0

	LIBS	+= -framework Python
}

unix:!macx { # on Unix-ish platforms we should rely on pkgconfig
	# Python prior to 3.x doesn't register properly with pkg-config
	INCLUDEPATH	+= /usr/include/python2.6/
	LIBS		+= -lpython2.6

	# uncomment these lines (and comment those above) to use Python 3
#	CONFIG		+= link_pkgconfig
#	QMAKE_CXXFLAGS	+= -DPy_UNICODE_WIDE
#	PKGCONFIG	+= python3

	isEmpty(INSTALL_PREFIX):INSTALL_PREFIX = /usr/local
	isEmpty(TW_PLUGINPATH):TW_PLUGINPATH = $$INSTALL_PREFIX/lib/texworks
	target.path	= $$TW_PLUGINPATH
	INSTALLS	+= target
}

win32 { # paths here are specific to my setup
	INCLUDEPATH += c:/MinGW514/local/include
	INCLUDEPATH += c:/Python26/include

	LIBS += -Lc:/MinGW514/local/lib
	LIBS += -Lc:/Python26/libs -lpython26
}


# Input
HEADERS	+=	TWPythonPlugin.h \
			../../src/TWScript.h \
			../../src/TWScriptAPI.h \
			../../src/ConfigurableApp.h \
			../../src/TWSystemCmd.h

SOURCES	+=	TWPythonPlugin.cpp \
			../../src/TWScript.cpp \
			../../src/TWScriptAPI.cpp
