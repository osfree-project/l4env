PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= con_evdemo
LIBS		= -lcon -lconstream-server
DEFAULT_RELOC	= 0x00800000

SRC_C		= evdemo.c
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/prog.mk
