SYSTEMS         := x86-l4v2 amd64-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = cpuwaste
DEFAULT_RELOC    = 0x00ae0000
LIBS             = -ll4dope
SRC_C            = cpuwaste.c

include $(L4DIR)/mk/prog.mk
