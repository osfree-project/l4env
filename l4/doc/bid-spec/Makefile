L4DIR   ?= ../..

SPEC = spec.tex
SRC_TEX = $(SPEC) abstract.tex
SRC_HTML = bid-spec.tex

SRC_DOX	 = $(SRC_HTML:.tex=.cfg)
LANG_TEX = config-language.tex

all::

$(SRC_DOX:.cfg=.title):
	@echo guide >$@
	@echo "Building Infrastructure for DROPS (BID) Specification" >>$@

$(SRC_HTML:.tex=): $(SRC_HTML) $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf)
	$(VERBOSE)$(RM) -r $@
	$(VERBOSE)install -d $@/html
	$(VERBOSE)latex2html -local_icons -split 5 -link 3\
		-show_section_numbers $(SRC_HTML) -dir $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.ps) $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.pdf) $@/html
	$(VERBOSE)touch $@

$(SRC_HTML): $(SPEC:.tex=.dvi)
	$(VERBOSE)./tex-preprocess.pl $(<:.dvi=.tex) >$@
	$(VERBOSE)cp $(<:.dvi=.aux) $(@:.tex=.aux)
	$(VERBOSE)cp $(<:.dvi=.bbl) $(@:.tex=.bbl)

clean::
	$(VERBOSE)$(RM) -rv $(SRC_HTML) $(SRC_HTML:.tex=.aux) \
			$(SRC_HTML:.tex=.bbl)

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

