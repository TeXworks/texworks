libs := poppler-qt4 poppler
inc_dirs := /usr/X11/include
qt_libs := $(shell pkg-config QtCore QtGui QtXml --cflags --libs)
SRCS := $(wildcard *.cpp)

CC = g++

all: pdf_viewer

moc_PDFView.o : PDFView.h
	moc PDFView.h > moc_PDFView.cpp
	$(CC) -g -O0 \
	  $(addprefix -I,$(inc_dirs)) $(addprefix -l,$(libs)) \
		$(qt_libs) \
	  -c moc_PDFView.cpp

PDFView.o : PDFView.cpp
	$(CC) -g -O0 \
	  $(addprefix -I,$(inc_dirs)) $(addprefix -l,$(libs)) \
		$(qt_libs) \
	  -c PDFView.cpp

pdf_viewer: PDFView.o moc_PDFView.o poppler_viewer.cpp
	$(CC) -g -O0 \
	  $(addprefix -I,$(inc_dirs)) $(addprefix -l,$(libs)) \
		$(qt_libs) \
	  -o pdf_viewer moc_PDFView.o PDFView.o poppler_viewer.cpp


.PHONY: viewer

