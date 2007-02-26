SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = testnit
DEFAULT_RELOC    = 0x00b80000
SRC_C            = testnit.c

CLIENTIDL        = nitpicker.idl
SERVERIDL        = nitevent.idl

include $(L4DIR)/mk/prog.mk

