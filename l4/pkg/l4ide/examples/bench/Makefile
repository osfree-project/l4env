# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

PRIVATE_INCDIR = $(PKGDIR)/include

# source files
SRC_C =		main.c

# target
TARGET =	l4ide_bench
MODE =		l4env
SYSTEMS =	x86-l4v2
DEFAULT_RELOC = 0x00a80000

# don't know what is really needed
LIBS =		-lgeneric_blk

# include prog role
include $(L4DIR)/mk/prog.mk

