PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2
SRC_C          = main.c
LIBS           += -lore 

DEFAULT_RELOC  = 0x00980000
TARGET         = ore_debug

include $(L4DIR)/mk/prog.mk
