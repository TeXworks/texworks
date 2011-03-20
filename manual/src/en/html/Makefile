LANG=$(notdir $(abspath ..))

Logos = images/Linux.png images/Mac.png images/Windows.png images/TeXworks.png images/example.png
EpsFromPdf = images/MacCmdKey.eps images/MacOptKey.eps images/interface-summary.eps images/consoleOutput.eps images/errorParsingScript.eps images/LMB.eps images/RMB.eps
EpsFromPng = images/toolbar1.eps images/toolbar2.eps images/iconTypeset.eps images/iconAbortTypesetting.eps images/replaceDialog.eps images/screenshotHardWrapDlg.eps images/TeXworks.eps images/Linux.eps images/Mac.eps images/Windows.eps

IMAGES = $(Logos) $(EpsFromPdf) $(EpsFromPng)

HTOPTS = "manual,info,2,imgdir:images/,sec-filename,next,index=2,url-enc" "" "-S*"

WINDOWS = $(if $(filter .exe,$(suffix $(SHELL))),"y")
SUPRESSOUT = > $(if $(WINDOWS),nul,/dev/null)
CP = $(if $(WINDOWS),copy,cp)

.PHONY : html clean dist-clean dist dist-pdf dist-html

SOURCES = index.tex $(filter-out manual.tex,$(subst ../,,$(wildcard ../*.tex ../*.sty)))

html : index.html

index.html : $(IMAGES) $(SOURCES) index.ind manual.cfg tex4ht.env
	@htlatex index $(HTOPTS)
	@htlatex index $(HTOPTS)

%.tex : ../%.tex
	$(CP) $< $@
%.sty : ../%.sty
	$(CP) $< $@
index.tex : ../manual.tex
	$(CP) $< $@

images/Linux.png : ../images/Linux.pdf
	@mkdir -p images
	@convert ../images/Linux.pdf -resize 48x48 images/Linux.png $(SUPRESSOUT)
images/Mac.png : ../images/Mac.pdf
	@mkdir -p images
	@convert ../images/Mac.pdf -resize 48x48 images/Mac.png $(SUPRESSOUT)
images/Windows.png : ../images/Windows.pdf
	@mkdir -p images
	@convert ../images/Windows.pdf -resize 48x48 images/Windows.png $(SUPRESSOUT)
images/TeXworks.png : ../images/TeXworks.png
	@mkdir -p images
	@convert ../images/TeXworks.png -resize 100x100 images/TeXworks.png $(SUPRESSOUT)
images/example.png : ../images/example.pdf
	@mkdir -p images
	@convert -density 300 "$<"  -resize 48x48 "$@" $(SUPRESSOUT)
images/%.eps : ../images/%.pdf
	@echo "Creating image $@ (from $<)"
	@mkdir -p images
	@gs -dSAFER -dBATCH -dNOPAUSE -sDEVICE=epswrite -sOutputFile="$@" "$<" $(SUPRESSOUT)
images/%.eps : ../images/%.png
	@echo "Creating image $@ (from $<)"
	@mkdir -p images
	@convert "$<" "$@" $(SUPRESSOUT)


index.ind : images $(SOURCES) manual.cfg tex4ht.env
	@echo "Creating index..."
	@echo "   running htlatex..."
	@htlatex index $(HTOPTS)
	@echo "   creating input file..."
	@tex '\def\filename{{index}{idx}{4dx}{ind}} \input  idxmake.4ht'
	@echo "   running makeindex..."
	@makeindex -o index.ind index.4dx

clean : 
	rm -f *.tex *.sty idxmake.* *.aux *.log
	rm -f index.4ct index.4tc index.dvi index.idv index.idx index.lg index.log index.out index.tmp index.xref index.4dx index.4ix index.ilg index.ind

dist-clean : clean
	rm -f *.html index.css
	rm -rf images/

dist : dist-html

dist-html : index.html tidy.cfg manual.css
	@rm -rf TeXworks-manual/
	@mkdir -p ../../../html/TeXworks-manual/$(LANG)/images
	@$(CP) images/*.png ../../../html/TeXworks-manual/$(LANG)/images
	@$(CP) *.html *.css tw-help-title.txt ../../../html/TeXworks-manual/$(LANG)/
	@tidy -config tidy.cfg ../../../html/TeXworks-manual/$(LANG)/*.html || true

