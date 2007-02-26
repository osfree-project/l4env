PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2
SRC_C_uip_ore_client	   	   = client.c
SRC_C_uip_ore_server			= server.c
LIBS           += -luip_ore -lore

DEFAULT_RELOC  = 0x00980000
TARGET         = uip_ore_server uip_ore_client

include $(L4DIR)/mk/prog.mk
