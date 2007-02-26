PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= l4rm_exc
DEFAULT_RELOC	= 0x00A00000
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= main.c
SRC_S		= exc_entry.S

include $(L4DIR)/mk/prog.mk
