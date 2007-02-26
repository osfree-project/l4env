PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

MODE           = l4env_base
TARGET         = healing_http
DEFAULT_RELOC  = 0x01ad0000
SRC_C          = main.c 
SYSTEMS        = x86-l4v2

DEPENDS_PKGS    = l4vfs

#UIP_DIR		= $(PKGDIR)/lib/uip/contrib/uip
#UIP_ORE_DIR = $(PKGDIR)/lib/uip/uip-ore

#vpath %.c	 $(UIP_DIR) $(UIP_ORE_DIR) $(PKGDIR)/lib/uip/mini_http_uip

LIBS         += -lparsecmdline -luip_ore -lore -lferret_monitor 	\
				-lferret_consumer -lferret_util						\
				-lferret_client -lferret_producer

# no unused warnings, because the UIP stack contains
# two unused labels and I do not want to change it
WARNINGS	+= -Wno-unused

include $(L4DIR)/mk/prog.mk

