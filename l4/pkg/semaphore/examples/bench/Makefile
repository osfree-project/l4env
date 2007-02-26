# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	semaphore_bench
MODE =		l4env
SYSTEMS =	x86-l4v2 x86-l4x0
DEFAULT_RELOC = 0x00400000
LIBS =		$(GCCLIB)

# include prog role
include $(L4DIR)/mk/prog.mk
