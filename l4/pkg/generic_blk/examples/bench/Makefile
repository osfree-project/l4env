# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	blk_bench
MODE =		l4env
SYSTEMS =	x86-l4v2
DEFAULT_RELOC = 0x00400000
LIBS =		-lgeneric_blk
PRIVATE_INCDIR = $(SRC_DIR)/../oskit

# include prog role
include $(L4DIR)/mk/prog.mk

