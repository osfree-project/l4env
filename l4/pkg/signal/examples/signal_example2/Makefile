#directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
DEFAULT_RELOC	= 0x00200000
TARGET		= signal_example2
SYSTEMS		= x86-l4v2

MODE		= l4env_base

WARNINGS	= -Wall

#DEBUG 		= 1

LIBS		= -nostdlib -lc_be_l4sig.o -lsignal

include $(L4DIR)/mk/prog.mk
