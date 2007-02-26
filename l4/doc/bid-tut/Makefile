L4DIR   ?= ../..

SRC_TEX = tut.tex
SRC_HTML = bid-tut.tex
SUPPORT = todo.sty own.bib tex-preprocess.pl

# we do not generate bid-tut.cfg, but we set it for doc.mk, which will
# install the html-files into $(L4DIR)/doc/html then.
SRC_DOX = $(SRC_HTML:.tex=.cfg)

all::

$(SRC_DOX:.cfg=.title):
	@echo guide >$@
	@echo "Building Infrastructure for DROPS (BID) Tutorial" >>$@

$(SRC_TEX:.tex=.dvi): $(SUPPORT)

$(SUPPORT):%:../bid-spec/%
	$(VERBOSE)ln -sf $< $@

$(SRC_HTML:.tex=): $(SRC_HTML) $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf)
	$(VERBOSE)$(RM) -r $@
	$(VERBOSE)install -d $@/html
	$(VERBOSE)latex2html -local_icons -split 5 -show_section_numbers \
		$(SRC_HTML) -dir $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.ps) $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.pdf) $@/html
	$(VERBOSE)touch $@

$(SRC_HTML): $(SRC_TEX:.tex=.dvi)
	$(VERBOSE)./tex-preprocess.pl $(<:.dvi=.tex) >$@
	$(VERBOSE)cp $(<:.dvi=.aux) $(@:.tex=.aux)
	$(VERBOSE)cp $(<:.dvi=.bbl) $(@:.tex=.bbl)

clean::
	$(VERBOSE)$(RM) -rv $(SRC_HTML) $(SRC_HTML:.tex=.aux) \
			$(SRC_HTML:.tex=.bbl) $(SUPPORT)

cleanall::
	$(VERBOSE)$(RM) -rv $(SRC_HTML:.tex=)

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

