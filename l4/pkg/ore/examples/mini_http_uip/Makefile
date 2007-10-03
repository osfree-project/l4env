PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = mini_http_uip
DEFAULT_RELOC  = 0x01ad0000
SRC_C          = main.c log.c oredev.c uip.c uip_arch.c uip_arp.c 
SYSTEMS        = x86-l4v2

DEPENDS_PKGS    = l4vfs

UIP_DIR		= $(PKGDIR)/lib/uip/contrib/uip
UIP_ORE_DIR = $(PKGDIR)/lib/uip/uip-ore

vpath %.c	 $(UIP_DIR) $(UIP_ORE_DIR) $(PKGDIR)/lib/uip/mini_http_uip

LIBS         += -lore -lparsecmdline

# no unused warnings, because the UIP stack contains
# two unused labels and I do not want to change it
WARNINGS	+= -Wno-unused

PRIVATE_INCDIR = $(PKGDIR)/examples/mini_http_uip $(UIP_DIR) $(UIP_ORE_DIR)

include $(L4DIR)/mk/prog.mk

