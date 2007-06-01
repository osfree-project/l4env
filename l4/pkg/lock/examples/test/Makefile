# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	lock_test
MODE =		l4env
SYSTEMS =	x86-l4v2 arm-l4v2
DEFAULT_RELOC = 0x00400000

# include prog role
include $(L4DIR)/mk/prog.mk
