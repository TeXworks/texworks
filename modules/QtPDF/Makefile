libs := poppler-qt4 poppler
inc_dirs := /usr/X11/include
qt_libs := $(shell pkg-config QtCore QtGui QtXml --cflags --libs)
SRCS := $(wildcard *.cpp)

CC = g++

all: pdf_viewer

pdf_viewer: $(SRCS)
	$(CC) -g -O0 \
	  $(addprefix -I,$(inc_dirs)) $(addprefix -l,$(libs)) \
		$(qt_libs) \
	  -o pdf_viewer $(SRCS)


.PHONY: viewer

