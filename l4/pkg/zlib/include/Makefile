PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

LIBZ		:= $(PKGDIR_ABS)/lib/dist
EXPORTED	:= zconf.h zlib.h
EXPORTED_O      := $(addprefix $(OBJ_DIR)/, $(EXPORTED))
INCSRC_DIR      := $(OBJ_DIR)
TARGET          := $(EXPORTED)

all:: $(EXPORTED_O)

$(EXPORTED_O):$(OBJ_DIR)/%: $(LIBZ)/%
	@$(INSTALL_LOCAL_MESSAGE)
	$(VERBOSE)$(LN) -fs $< $@

cleanall::
	$(VERBOSE)rm -f $(EXPORTED_O)

include $(L4DIR)/mk/include.mk
