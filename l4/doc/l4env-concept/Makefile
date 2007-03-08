L4DIR  ?= ../..

include $(L4DIR)/mk/Makeconf

ifeq ($(IN_OBJ_DIR),)

all install clean cleanall:: $(OBJ_DIR)/Makefile.build
	$(MAKE) -C $(OBJ_DIR) O=$(OBJ_BASE) -f Makefile.build $@

$(OBJ_DIR)/Makefile.build: $(SRC_DIR)/Makefile
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/l4env.fig $(OBJ_DIR)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/dataspaces.fig $(OBJ_DIR)
	$(VERBOSE)$(LN) -sf $(SRC_DIR)/own.bib $(OBJ_DIR)
	$(VERBOSE)echo 'IN_OBJ_DIR=1'                 > $@
	$(VERBOSE)echo 'L4DIR=$(L4DIR_ABS)'          >> $@
	$(VERBOSE)echo 'SRC_DIR=$(SRC_DIR)'          >> $@
	$(VERBOSE)echo 'OBJ_BASE=$(OBJ_BASE)'        >> $@
	$(VERBOSE)echo 'vpath %.tex $(SRC_DIR)'      >> $@
	$(VERBOSE)echo 'include $(SRC_DIR)/Makefile' >> $@

else

SRC_TEX = l4env.tex
PICS	= l4env.eps dataspaces.eps

all::
l4env.dvi: $(PICS)
clean::
	$(VERBOSE)$(RM) $(PICS)

cleanall::
	$(VERBOSE)$(RM) l4env.fig dataspaces.fig own.bib

include $(L4DIR)/mk/doc.mk

endif
