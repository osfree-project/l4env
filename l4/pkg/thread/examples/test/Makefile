# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	thread_test
MODE =		l4env
SYSTEMS =	x86-l4v2 arm-l4x0
DEFAULT_RELOC_x86 = 0x00400000
DEFAULT_RELOC_arm = 0x00300000

# include prog role
include $(L4DIR)/mk/prog.mk
