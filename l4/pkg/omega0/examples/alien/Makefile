PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2
TARGET	= omega0-alien
SRC_C	= main.c pit.c pic.c
DEFAULT_RELOC = 0x00A00000

MODE	= sigma0
LIBS	= -lomega0

PRIVATE_INCDIR = $(PKGDIR)/examples/pit
vpath %.c $(PKGDIR)/examples/pit
include $(L4DIR)/mk/prog.mk
