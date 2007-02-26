PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

TARGET    = libuip_ore.a
SYSTEMS   = x86-l4v2
SRC_C     = uip.c uip_arch.c uip_arp.c 				\
			log.c oredev.c uip-thread.c uip_rxtx.c uip_ports.c	

PRIVATE_INCDIR	= 	$(PKGDIR)/lib/uip/uip-ore 		\
					$(PKGDIR)/lib/uip/contrib/uip

VPATH = $(PKGDIR)/lib/uip/contrib/uip $(PKGDIR)/lib/uip/uip_ore

WARNINGS += -Wno-unused-label

include $(L4DIR)/mk/lib.mk
