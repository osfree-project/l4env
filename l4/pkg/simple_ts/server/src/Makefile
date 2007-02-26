PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 x86-l4x0

TARGET		= simple_ts
DEFAULT_RELOC	= 0x01400000

SRC_C		= main.c
LIBS		= -lgeneric_ts-server

include $(L4DIR)/mk/prog.mk
