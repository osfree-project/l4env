PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= periodic periodic_self
DEFAULT_RELOC	= 0x01a00000

SRC_C_periodic	= main.c
SRC_C_periodic_self = self.c
LIBS		= -lparsecmdline -lcpu_reserve

include $(L4DIR)/mk/prog.mk
