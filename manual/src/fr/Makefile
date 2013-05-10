LANG=$(notdir $(abspath .))

SYNCTEX = -synctex=1

WINDOWS = $(if $(filter .exe,$(suffix $(SHELL))),"y")
SUPRESSOUT = > $(if $(WINDOWS),nul,/dev/null)
CP = $(if $(WINDOWS),copy,cp)

.PHONY : all pdf html clean dist-clean dist dist-pdf dist-html recursive-clean recursive-dist-clean

all : pdf html

pdf : manuel.pdf

html :
	@$(MAKE) -C html

manuel.pdf : manuel.ind *.tex *.sty images/*.pdf images/*.png
	@lualatex manuel
	@lualatex manuel
	@lualatex $(SYNCTEX) manuel
	
manuel.ind : *.tex *.sty
	@lualatex manuel
	@makeindex manuel

clean :
	rm -f *.aux *.log *.out *.idx *.ind *.ilg *.toc

dist-clean : clean
	rm -f manuel.pdf *.synctex *.synctex.gz

recursive-clean : clean
	@$(MAKE) -C html clean

recursive-dist-clean : dist-clean
	@$(MAKE) -C html dist-clean

dist : dist-pdf dist-html

dist-pdf : manuel.pdf
	@mkdir -p ../../pdf/$(LANG)
	@$(CP) manuel.pdf ../../pdf/$(LANG)/TeXworks-manuel-$(LANG).pdf

dist-html :
	$(MAKE) -C html dist
