CXX ?= g++

CXXFLAGS := -g -O0 -I. -I./src $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --cflags)
# Debugging output?
CXXFLAGS += -DDEBUG
LDFLAGS := $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --libs)

VPATH = src
SRCS := main.cpp PDFViewer.cpp moc_PDFViewer.cpp PDFDocumentView.cpp moc_PDFDocumentView.cpp

all: pdf_viewer

pdf_viewer: $(SRCS:.cpp=.o) icons.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o pdf_viewer $^

%.cpp : %.qrc
	rcc -o $@ $<

moc_%.cpp: %.h
	moc $< > $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	git clean -fdx

.PHONY: all clean

