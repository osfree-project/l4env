PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

LIBZ		:= $(PKGDIR)/lib/dist
EXPORTED	:= zconf.h zlib.h

all:: $(EXPORTED)

$(foreach f,$(EXPORTED),$(f)) : % : $(LIBZ)/%
	@$(INSTALL_LOCAL_MESSAGE)
	$(VERBOSE)$(LN) -s $< .

cleanall::
	$(VERBOSE)rm -f $(EXPORTED)

include $(L4DIR)/mk/include.mk





