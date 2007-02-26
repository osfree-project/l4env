SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = textedit
DEFAULT_RELOC    = 0x00ae0000
LIBS             = -ll4dope -lvscreen
SRC_C            = textedit.c
DPE_textedit.c   = textedit.dpe
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include

vpath % $(PKGDIR)/examples/textedit/common

include $(L4DIR)/mk/prog.mk
