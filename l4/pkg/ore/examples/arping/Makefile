# directories we need to know
PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2

TARGET		= ore_test_arping

SRC_C		= main.c 

CLIENTIDL 	= ore_rxtx.idl ore_manager.idl
LIBS	 	+= -lore -lparsecmdline -ldm_phys

DEFAULT_RELOC  = 0x00980000

include $(L4DIR)/mk/prog.mk
