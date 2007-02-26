PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= l4rm_test
DEFAULT_RELOC	= 0x00A00000
SYSTEMS		= x86-l4v2 x86-l4x0

LIBS		= $(GCCLIB)
SRC_C		= main.c
PRIVATE_INCDIR	= ../../../lib/include

include $(L4DIR)/mk/prog.mk
