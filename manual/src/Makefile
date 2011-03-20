LANGS = $(patsubst %/,manual-%,$(wildcard */))
DISTLANGS = $(patsubst manual-%,dist-%,$(LANGS))
CLEANLANGS = $(patsubst manual-%,clean-%,$(LANGS))
DISTCLEANLANGS = $(patsubst manual-%,dist-clean-%,$(LANGS))

.PHONY : all clean dist dist-clean $(LANGS) $(DISTLANGS) $(CLEANLANGS) $(DISTCLEANLANGS)

all : $(LANGS)

clean : $(CLEANLANGS)

dist : $(DISTLANGS)

dist-clean : $(DISTCLEANLANGS)

$(LANGS) : 
	@$(MAKE) -C $(patsubst manual-%,%,$@)

$(DISTLANGS) :
	@$(MAKE) -C $(patsubst dist-%,%,$@) dist

$(CLEANLANGS) :
	@$(MAKE) -C $(patsubst clean-%,%,$@) recursive-clean

$(DISTCLEANLANGS) :
	@$(MAKE) -C $(patsubst dist-clean-%,%,$@) recursive-dist-clean

