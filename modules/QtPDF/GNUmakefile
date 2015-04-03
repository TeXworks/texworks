CXX ?= g++

CXXFLAGS := -g -O0 -Wall -I. -I./src $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --cflags)
# Debugging output?
CXXFLAGS += -DDEBUG -DQT_NO_CAST_FROM_ASCII -DQT_NO_CAST_TO_ASCII -DQT_NO_CAST_FROM_BYTEARRAY

LDFLAGS := $(shell pkg-config freetype2 poppler poppler-qt4 QtCore QtGui QtXml --libs)
LDFLAGS += -lfitz -lmupdf -ljbig2dec -ljpeg -lopenjpeg -lz

VPATH = src src/backends
SRCS := main.cpp PDFViewer.cpp moc_PDFViewer.cpp\
  PDFDocumentView.cpp moc_PDFDocumentView.cpp PDFBackend.cpp moc_PDFBackend.cpp

# Select backend
CXXFLAGS += -DUSE_POPPLER
SRCS += PopplerBackend.cpp moc_PopplerBackend.cpp
#CXXFLAGS += -DUSE_MUPDF
#SRCS += MuPDFBackend.cpp moc_MuPDFBackend.cpp


OBJS = $(SRCS:.cpp=.o)

all: pdf_viewer

pdf_viewer: $(OBJS) icons.cpp
	$(CXX) $(CXXFLAGS) $(LDFLAGS) -o pdf_viewer $^

%.cpp : %.qrc
	rcc -o $@ $<

moc_%.cpp: %.h
	moc $< > $@

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $<

clean :
	rm -f pdf_viewer $(OBJS) icons.cpp

docs :
	cd docs && rake docs

.PHONY: all clean docs

