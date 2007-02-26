#directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
DEFAULT_RELOC	= 0x00200000
TARGET		= signal_fpe
SYSTEMS		= x86-l4v2
CFLAGS		= -Wno-div-by-zero

MODE		= l4env_base

WARNINGS	= -Wall

LIBS		= -lc_be_l4sig.o -lsignal

include $(L4DIR)/mk/prog.mk
