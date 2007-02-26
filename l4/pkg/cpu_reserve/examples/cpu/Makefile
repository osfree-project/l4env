PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= cpu
DEFAULT_RELOC	= 0x01900000

SRC_C		= main.c
LIBS		= -lparsecmdline -lcpu_reserve

include $(L4DIR)/mk/prog.mk
