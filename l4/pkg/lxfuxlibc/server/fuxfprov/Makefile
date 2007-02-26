PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= fuxfprov
MODE		= l4env
DEFAULT_RELOC	= 0x00500000
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= main.c
LIBS		= -lclxfux.o -ll4z -lstdio_zlib_ux -lgeneric_fprov-server

include $(L4DIR)/mk/prog.mk

