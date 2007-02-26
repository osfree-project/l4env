# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# libraries
MALLOC_BACKEND = -lc_be_buddy_slab_mem

# target
TARGET =        malloc_stress
MODE =		l4env_minimal
DEFAULT_RELOC = 0x00800000
SYSTEMS =       x86-l4v2 x86-l4x0

# include prog rule
include $(L4DIR)/mk/prog.mk
