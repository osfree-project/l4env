PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= logcon
LIBS		= -u putchar -u puts -lcontxt -lcon -lconstream-server
DEFAULT_RELOC	= 0x00300000

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
