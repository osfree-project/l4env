L4DIR	?= ../..

include $(L4DIR)/mk/Makeconf

ifeq ($(IN_OBJ_DIR),)

all install clean cleanall:: $(OBJ_DIR)/Makefile.build
	$(MAKE) -C $(OBJ_DIR) O=$(OBJ_BASE) -f Makefile.build $@

$(OBJ_DIR)/Makefile.build: $(SRC_DIR)/Makefile
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/building.tex $(OBJ_DIR)
	$(VERBOSE)echo 'IN_OBJ_DIR=1'                 > $@
	$(VERBOSE)echo 'L4DIR=$(L4DIR_ABS)'          >> $@
	$(VERBOSE)echo 'SRC_DIR=$(SRC_DIR)'          >> $@
	$(VERBOSE)echo 'OBJ_BASE=$(OBJ_BASE)'        >> $@
	$(VERBOSE)echo 'vpath %.tex $(SRC_DIR)'      >> $@
	$(VERBOSE)echo 'include $(SRC_DIR)/Makefile' >> $@

else

SRC_TEX	= building.tex
SRC_HTML= drops-building.tex

SRC_DOX  = $(SRC_HTML:.tex=.cfg)
SUPPORT = todo.sty
all::

$(OBJ_DIR)/$(SRC_DOX:.cfg=.title):
	@echo guide >$@
	@echo "Building DROPS HOWTO" >>$@

$(SRC_TEX:.tex=.dvi): $(SUPPORT)

$(SUPPORT):%:../bid-spec/%
	$(VERBOSE)ln -sf $< $@

$(SRC_HTML:.tex=): $(SRC_HTML) $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf)
	$(VERBOSE)$(RM) -r $@
	$(VERBOSE)install -d $@/html
	$(VERBOSE)latex2html -local_icons -split 4\
		-show_section_numbers $(SRC_HTML) -dir $@/html
	$(VERBOSE)cp $(SRC_TEX:.tex=.ps) $(SRC_TEX:.tex=.pdf) $@/html
	$(VERBOSE)chmod 644 $@/html/*
	$(VERBOSE)touch $@

$(SRC_HTML): $(SRC_TEX:.tex=.dvi)
	$(VERBOSE)cp $(<:.dvi=.tex) $@
	$(VERBOSE)cp $(<:.dvi=.aux) $(@:.tex=.aux)
	$(VERBOSE)test ! -r $(<:.dvi=.bbl) || cp $(<:.dvi=.bbl) $(@:.tex=.bbl)

include $(L4DIR)/mk/doc.mk

clean::
	$(VERBOSE)$(RM) -rv $(SRC_HTML) $(SRC_HTML:.tex=.aux) \
		$(SRC_HTML:.tex=.bbl) $(SUPPORT) building.tex

endif
