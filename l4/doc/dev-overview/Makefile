# FIXME: merge gosh support into l4/mk/doc.mk?


PKGDIR  ?= .
L4DIR	?= ../..

include $(L4DIR)/mk/Makeconf

ifeq ($(IN_OBJ_DIR),)

all clean cleanall install:: $(OBJ_DIR)/Makefile.build
	$(VERBOSE)make -C $(OBJ_DIR)  O=$(OBJ_BASE) -f $(OBJ_DIR)/Makefile.build $@

$(OBJ_DIR)/Makefile.build: $(SRC_DIR)/Makefile
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)echo 'IN_OBJ_DIR=1'                 > $@
	$(VERBOSE)echo 'L4DIR=$(L4DIR_ABS)'          >> $@
	$(VERBOSE)echo 'SRC_DIR=$(SRC_DIR)'          >> $@
	$(VERBOSE)echo 'OBJ_BASE=$(OBJ_BASE)'        >> $@
	$(VERBOSE)echo 'PKGDIR=$(PKGDIR_ABS)'        >> $@
	$(VERBOSE)echo 'vpath %.txt $(SRC_DIR)'      >> $@
	$(VERBOSE)echo 'include $(SRC_DIR)/Makefile' >> $@

else

TXTFILES = $(PKGNAME).txt 
IMGFILES = 
TITLE    = $(PKGNAME).title

PDFFILES  = $(TXTFILES:.txt=.pdf)
HTMLFILES = $(TXTFILES:.txt=.html)

TMPFILES  = $(TXTFILES:.txt=.dvi) $(TXTFILES:.txt=.aux) $(TXTFILES:.txt=.log) \
            $(TXTFILES:.txt=.blg) $(TXTFILES:.txt=.bbl)
INSTFILES = $(PDFFILES) $(HTMLFILES) $(TITLE)


all:: $(PDFFILES) $(HTMLFILES) $(TITLE)
	$(VERBOSE)install -d $(OBJ_BASE)/doc/html/$(PKGNAME)
	$(VERBOSE)$(LN) -sf $(OBJ_DIR)/$(PDFFILES) $(OBJ_BASE)/doc/html/$(PKGNAME)/
	$(VERBOSE)$(LN) -sf $(OBJ_DIR)/$(HTMLFILES) $(OBJ_BASE)/doc/html/$(PKGNAME)/index.html
	$(VERBOSE)$(LN) -sf $(OBJ_DIR)/$(TITLE) $(OBJ_BASE)/doc/html/$(TITLE)

clean::
	$(VERBOSE)rm -f $(TMPFILES) $(INSTFILES)

cleanall:: clean
	# do we need also to clean up in $(DROPS_STDDIR)? 
	$(VERBOSE)rm -rf $(OBJ_BASE)/doc/html/$(TITLE) $(OBJ_BASE)/doc/html/$(PKGNAME)

install:: all
	$(VERBOSE)install -d $(DROPS_STDDIR)/doc/html/$(PKGNAME)
	$(VERBOSE)install -m 0644 $(PDFFILES) $(DROPS_STDDIR)/doc/html/$(PKGNAME)/
	$(VERBOSE)install -m 0644 $(HTMLFILES) $(DROPS_STDDIR)/doc/html/$(PKGNAME)/index.html
	$(VERBOSE)install -m 0644 $(TITLE) $(DROPS_STDDIR)/doc/html/

$(TITLE):
	$(VERBOSE)echo -en "guide\nDeveloping with L4 -- Overview and Pointers" > $(PKGNAME).title

%.pdf: %.tex
	$(VERBOSE)pdflatex $<
	$(VERBOSE)pdflatex $<
	$(VERBOSE)pdflatex $<

%.html: %.txt $(TXTFILES)
	$(VERBOSE)$(GOSH) --style $(GOSHDIR)/html.gosh $< > $@ || rm $@

%.tex: %.txt
	$(VERBOSE)$(GOSH) $(GOSH2TEX_OPT) $< > $@ || rm $@

endif

