#directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c
SRC_S =		entries.S

# target
DEFAULT_RELOC	= 0x00200000
TARGET		= sig_exc_problem
SYSTEMS		= x86-l4v2
CFLAGS		= -Wno-div-by-zero

MODE		= l4env_base

LIBS		= -lc_be_l4sig.o -lsignal

WARNINGS	= -Wall

include $(L4DIR)/mk/prog.mk
