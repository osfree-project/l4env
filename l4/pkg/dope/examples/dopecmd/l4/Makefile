SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = dopecmd
DEFAULT_RELOC    = 0x00a80000
LIBS             = -ll4dope 
SRC_C            = dopecmd.c startup.c
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/dopecmd/include \
                   $(PKGDIR)/include

vpath % $(PKGDIR)/examples/dopecmd/common 

include $(L4DIR)/mk/prog.mk

dopecmd.o: simpleterm.i

simpleterm.i: simpleterm.dpe
	$(PKGDIR)/tool/gen_dopecode $^ > $@

