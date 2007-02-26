SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = ollepetze
DEFAULT_RELOC    = 0x00a80000
LIBS             = -ll4dope
SRC_C            = ollepetze.c
CLIENTIDL        = petze.idl

include $(L4DIR)/mk/prog.mk
