PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= events
SYSTEMS 	= x86-l4v2
MODE		= sigma0
DEFAULT_RELOC	= 0x01540000
PRIVATE_INCDIR	= $(PKGDIR)/server/include .
LIBS		= -lparsecmdline

SRC_C		= server.c lib.c dump.c mem_lock.c

include $(L4DIR)/mk/prog.mk
