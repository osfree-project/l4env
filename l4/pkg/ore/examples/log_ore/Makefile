#
# Makefile for sources of log server, net version
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET		= log_ore
DEFAULT_RELOC	= 0x00300000
MODE		= l4env_minimal

SRC_C		= server.c #muxed.c
			  
VPATH		= $(PKGDIR_ABS)/lib/uip/contrib/uip	\
		  $(PKGDIR_ABS)/lib/uip/uip-ore		\
		  $(PKGDIR_ABS)/lib/uip/log_ore

LIBS		+= -lparsecmdline -luip_ore -lore

PRIVATE_INCDIR	= $(PKGDIR_ABS)/lib/uip/contrib/uip 	\
		  $(PKGDIR_ABS)/lib/uip/log_ore		\
		  $(PKGDIR_ABS)/lib/uip/uip-ore		\
		  $(L4DIR_ABS)/pkg/log/server/include

SERVERIDL	= log.idl
IDL_PKGDIR	= $(OBJ_BASE)/pkg/log

WARNINGS	+= -Wno-unused

include $(L4DIR)/mk/prog.mk
