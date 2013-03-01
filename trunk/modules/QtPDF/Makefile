CXX ?= g++

CXXFLAGS := -g -O0 -I. $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --cflags)
LDFLAGS := $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --libs)

SRCS := $(wildcard *.cpp)
MOC_HDRS := $(wildcard *.h)
MOC_SRCS := $(addprefix moc_,$(MOC_HDRS:.h=.cpp))

all: pdf_viewer

pdf_viewer: $(SRCS:.cpp=.o) $(MOC_SRCS:.cpp=.o) icons.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o pdf_viewer $^

icons.cpp : icons.qrc
	rcc -o $@ $<

moc_%.cpp: %.h
	moc $< > $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	git clean -fdx

.PHONY: all clean

