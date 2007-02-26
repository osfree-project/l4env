SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = minitop
DEFAULT_RELOC    = 0x00ae0000
LIBS             = -ll4dope 
SRC_C            = minitop.c startup.c getvalues.c thread.c
DPE_minitop.c    = minitop.dpe
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/minitop/include \
                   $(PKGDIR)/include

vpath % $(PKGDIR)/examples/minitop/common 

include $(L4DIR)/mk/prog.mk
