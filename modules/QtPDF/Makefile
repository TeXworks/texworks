CXX ?= g++

CXXFLAGS := -g -O0 -I. -I./src $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --cflags)
LDFLAGS := $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --libs)

SRCS := $(wildcard *.cpp) $(wildcard ./src/*.cpp)
MOC_SRCS := $(patsubst %.h,moc_%.cpp,$(wildcard *.h)) $(patsubst src/%.h,src/moc_%.cpp,$(wildcard src/*.h))

all: pdf_viewer

pdf_viewer: $(SRCS:.cpp=.o) $(MOC_SRCS:.cpp=.o) icons.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o pdf_viewer $^

icons.cpp : icons.qrc
	rcc -o $@ $<

moc_%.cpp: %.h
	cd `dirname $<` && moc `basename $<` > `basename $@`

%.o: %.cpp
	cd `dirname $<` && $(CXX) $(CXXFLAGS) -c `basename $<`

clean :
	git clean -fdx

.PHONY: all clean

