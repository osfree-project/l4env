PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= cpuclt
DEFAULT_RELOC	= 0x01a00000

SRC_C		= main.c
LIBS		= -lparsecmdline -lcpu_reserve

include $(L4DIR)/mk/prog.mk
