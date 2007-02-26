PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= con_demo1
LIBS		= -lcon
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= demo1.c
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/prog.mk
