SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = dopecmd
DEFAULT_RELOC    = 0x00ae0000
LIBS             = -ll4dope -lvscreen
SRC_C            = dopecmd.c startup.c
DPE_dopecmd.c    = simpleterm.dpe
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/dopecmd/include \
                   $(PKGDIR)/include

vpath %.c $(PKGDIR)/examples/dopecmd/common 
vpath %.dpe $(PKGDIR)/examples/dopecmd/common 

include $(L4DIR)/mk/prog.mk
