# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	slab_test
MODE =		sigma0
SYSTEMS =	x86-l4v2 arm-l4x0
DEFAULT_RELOC = 0x00440000
LIBS = -lslab -llist_alloc -ll4env_err

# include prog role
include $(L4DIR)/mk/prog.mk
