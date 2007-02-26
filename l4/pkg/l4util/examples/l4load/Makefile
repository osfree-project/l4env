# directories we need to know
PKGDIR 		?= ../..
L4DIR		?= $(PKGDIR)/../..

# source files
SRC_C		= main.c

# target
TARGET		= l4load
SYSTEMS		= x86-l4v2 x86-l4x0
DEFAULT_RELOC	= 0x00350000
LIBS		= -lparsecmdline

include $(L4DIR)/mk/prog.mk
