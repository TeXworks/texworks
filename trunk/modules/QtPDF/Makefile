libs := poppler-qt4 poppler
inc_dirs := /usr/X11/include
qt_files := $(shell pkg-config QtCore QtGui QtXml --libs --cflags)
SRCS := $(wildcard *.cpp)

CC = g++

all: pdf_viewer

pdf_viewer: PDFDocumentView.o moc_PDFDocumentView.o PDFViewer.o moc_PDFViewer.o main.cpp icons.cpp
	$(CC) -g -O0 \
	  $(addprefix -I,$(inc_dirs)) $(addprefix -l,$(libs)) \
		$(qt_files) \
	  -o pdf_viewer $^

icons.cpp : icons.qrc
	rcc -o $@ $^

moc_PDFDocumentView.cpp : PDFDocumentView.h
	moc PDFDocumentView.h > moc_PDFDocumentView.cpp

moc_PDFDocumentView.o : moc_PDFDocumentView.cpp
	$(CC) -g -O0 \
		$(qt_files) \
	  -c moc_PDFDocumentView.cpp

PDFDocumentView.o : PDFDocumentView.cpp
	$(CC) -g -O0 \
		$(qt_files) \
	  -c PDFDocumentView.cpp

moc_PDFViewer.cpp : PDFViewer.h
	moc PDFViewer.h > moc_PDFViewer.cpp

moc_PDFViewer.o : moc_PDFViewer.cpp
	$(CC) -g -O0 \
		$(qt_files) \
	  -c moc_PDFViewer.cpp

PDFViewer.o : PDFViewer.cpp
	$(CC) -g -O0 \
		$(qt_files) \
	  -c PDFViewer.cpp


.PHONY: viewer

