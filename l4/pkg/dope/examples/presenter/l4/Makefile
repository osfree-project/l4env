PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = presenter
SYSTEMS          = x86-l4v2 x86-l4x0
DEFAULT_RELOC    = 0x00c00000
LIBS             = -ll4dope -lvscreen
SRC_C            = presenter.c
PRIVATE_INCDIR   = $(PKGDIR)/server/include
#CLIENTIDL        = pslim.idl

include $(L4DIR)/mk/prog.mk
