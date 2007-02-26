# Makefile for omega0 delay example
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2
TARGET	= omega0_ex_delay
SRC_C	= main.c pit.c pic.c serial.c
MODE	= sigma0
LIBS	= -lomega0

DEFAULT_RELOC = 0x430000

include $(L4DIR)/mk/prog.mk
