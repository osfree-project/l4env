L4DIR   ?= ../..

include $(L4DIR)/mk/Makeconf

ifeq ($(IN_OBJ_DIR),)

all install clean cleanall:: $(OBJ_DIR)/Makefile.build
	$(MAKE) -C $(OBJ_DIR) O=$(OBJ_BASE) -f Makefile.build $@

$(OBJ_DIR)/Makefile.build: $(SRC_DIR)/Makefile
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/todo.sty $(OBJ_DIR)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/config-language.tex $(OBJ_DIR)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/own.bib $(OBJ_DIR)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/code $(OBJ_DIR)
	$(VERBOSE)echo 'IN_OBJ_DIR=1'                 > $@
	$(VERBOSE)echo 'L4DIR=$(L4DIR_ABS)'          >> $@
	$(VERBOSE)echo 'SRC_DIR=$(SRC_DIR)'          >> $@
	$(VERBOSE)echo 'OBJ_BASE=$(OBJ_BASE)'        >> $@
	$(VERBOSE)echo 'vpath %.tex $(SRC_DIR)'      >> $@
	$(VERBOSE)echo 'include $(SRC_DIR)/Makefile' >> $@

else


SPEC = spec.tex
SRC_TEX = $(SPEC) abstract.tex
SRC_HTML = bid-spec.tex

SRC_DOX	 = $(SRC_HTML:.tex=.cfg)
LANG_TEX = config-language.tex

$(OBJ_DIR)/$(SRC_DOX:.cfg=.title):
	$(VERBOSE)echo guide >$@
	$(VERBOSE)echo "Building Infrastructure for DROPS (BID) Specification" >>$@

$(SRC_HTML:.tex=): $(SRC_HTML) $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf)
	$(VERBOSE)$(RM) -r $@
	$(VERBOSE)install -d $@/html
	@$(GEN_MESSAGE)
	$(VERBOSE)latex2html -local_icons -split 5 -link 3 \
		-show_section_numbers $(SRC_HTML) -dir $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.ps) $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.pdf) $@/html
	$(VERBOSE)touch $@

$(SRC_HTML): $(SPEC:.tex=.pdf)
	@$(GEN_MESSAGE)
	$(VERBOSE)$(SRC_DIR)/tex-preprocess.pl $(SRC_DIR)/$(<:.pdf=.tex) > $@
	$(VERBOSE)cp $(<:.pdf=.aux) $(@:.tex=.aux)
	$(VERBOSE)cp $(<:.pdf=.bbl) $(@:.tex=.bbl)

clean::
	$(VERBOSE)$(RM) -rv $(wildcard $(SRC_HTML) $(SRC_HTML:.tex=.aux) \
			$(SRC_HTML:.tex=.bbl))

cleanall::
	$(VERBOSE)$(RM) $(wildcard todo.sty code config-language.tex own.bib)

include $(L4DIR)/mk/doc.mk

ifeq ($(HTMLDIR),)
pub::
	@echo 'Specify your http server directory in HTMLDIR in Makeconf.local'
	@echo 'Then, "make pub" will copy all the files into this directory'
	$(VERBOSE)exit 1
else
pub:: $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf) $(SRC_HTML:.tex=)
	$(VERBOSE)install -d $(HTMLDIR)
	$(VERBOSE)install $(SRC_HTML:.tex=)/html/* -m 664 $(HTMLDIR)
	$(VERBOSE)install $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf) -m 644 \
		$(HTMLDIR)
endif

help::
	@echo
	@echo "Additional targets"
	@echo "pub      - build html, ps and pdf version and publish"
	@echo "           this requires \"HTMLDIR\" to be set in Makeconf.local"

endif
