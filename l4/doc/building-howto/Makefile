L4DIR	?= ../..

SRC_TEX	= building.tex
SRC_HTML= drops-building.tex

SRC_DOX  = $(SRC_HTML:.tex=.cfg)
SUPPORT = todo.sty
all::

$(SRC_DOX:.cfg=.title):
	@echo guide >$@
	@echo "Building DROPS HOWTO" >>$@

$(SRC_TEX:.tex=.dvi): $(SUPPORT)

$(SUPPORT):%:../bid-spec/%
	$(VERBOSE)ln -sf $< $@

$(SRC_HTML:.tex=): $(SRC_HTML) $(SRC_TEX:.tex=.ps)
	$(VERBOSE)$(RM) -r $@
	$(VERBOSE)install -d $@/html
	$(VERBOSE)latex2html -local_icons -split 4\
		-show_section_numbers $(SRC_HTML) -dir $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.ps) $@/html
	$(VERBOSE)touch $@

$(SRC_HTML): $(SRC_TEX:.tex=.dvi)
	$(VERBOSE)cp $(<:.dvi=.tex) $@
	$(VERBOSE)cp $(<:.dvi=.aux) $(@:.tex=.aux)
	$(VERBOSE)test ! -r $(<:.dvi=.bbl) || cp $(<:.dvi=.bbl) $(@:.tex=.bbl)

include $(L4DIR)/mk/doc.mk

clean::
	$(VERBOSE)$(RM) -rv $(SRC_HTML) $(SRC_HTML:.tex=.aux) \
		$(SRC_HTML:.tex=.bbl) $(SUPPORT)
cleanall::
	$(VERBOSE)$(RM)
