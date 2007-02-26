PKGDIR          ?= ..
L4DIR           ?= $(PKGDIR)/../..

LIBPNG          := $(PKGDIR)/lib/dist
EXPORTED        := png.h pngasmrd.h pngconf.h

all:: $(EXPORTED)

$(foreach f,$(EXPORTED),$(f)) : % : $(LIBPNG)/%
	@$(INSTALL_LOCAL_MESSAGE)
	$(VERBOSE)$(LN) -s $< .

cleanall::
	$(VERBOSE)rm -f $(EXPORTED) 

include $(L4DIR)/mk/include.mk
