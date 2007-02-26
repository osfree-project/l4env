PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2
SRC_C_ore_test  = main.c
SRC_C_ore_send	= ore_send.c
#SRC_C_ore_recv	= ore_recv.c
#CLIENTIDL      = ore_manager.idl ore_rxtx.idl
LIBS           += -lore 

DEFAULT_RELOC  = 0x00980000
TARGET         = ore_test ore_send #ore_recv

include $(L4DIR)/mk/prog.mk
