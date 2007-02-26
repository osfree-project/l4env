PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pcidevs
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= main.c
LIBS		= -lpci24-single

include $(L4DIR)/mk/prog.mk
