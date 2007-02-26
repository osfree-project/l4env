# directories we need to know
PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2 x86-l4x0

SRC_C          = main.c

CLIENTIDL      = ore.idl

LIBS           += -lore

DEFAULT_RELOC  = 0x00980000
TARGET         = ore_test_arping

#PRIVATE_INCDIR = $(LINUX24_INCDIR)

include $(L4DIR)/mk/prog.mk
