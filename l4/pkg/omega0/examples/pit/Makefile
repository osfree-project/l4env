PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2
TARGET	= pit
SRC_C	= main.c pit.c pic.c
DEFAULT_RELOC = 0x00A00000

MODE	= sigma0
LIBS	= -lomega0

include $(L4DIR)/mk/prog.mk
