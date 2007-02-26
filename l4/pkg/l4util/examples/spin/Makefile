# Makefile for performance measurement counter example

PKGDIR	= ../..
L4DIR	?= $(PKGDIR)/../..

TARGET		= spin spin1 spin2 spin3 spin4
DEFAULT_RELOC	= 0x00A00000
DEFAULT_RELOC_spin1= 0x00b00000
DEFAULT_RELOC_spin2= 0x00c00000
DEFAULT_RELOC_spin3= 0x00d00000
DEFAULT_RELOC_spin4= 0x00e00000
SRC_C		= main.c
LIBS		= -lparsecmdline
MODE		= sigma0

include $(L4DIR)/mk/prog.mk
