.PHONY : all

all : base14-fonts-locked.pdf jpg.pdf

base14-fonts-locked.pdf : base14-fonts.pdf
	pdftk $< output $@ compress user_pw 123 owner_pw test

%.pdf : %.tex
	pdflatex $<
