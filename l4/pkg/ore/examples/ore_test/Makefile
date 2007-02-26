PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2 x86-l4x0
SRC_C          = main.c
CLIENTIDL      = ore.idl
LIBS           += -lore

DEFAULT_RELOC  = 0x00980000
TARGET         = ore_test

include $(L4DIR)/mk/prog.mk
