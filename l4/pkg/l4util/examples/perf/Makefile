# Makefile for performance measurement counter example

PKGDIR	= ../..
L4DIR	?= $(PKGDIR)/../..

TARGET		= perf_p6
DEFAULT_RELOC	= 0x00A00000

CFLAGS		= -fno-defer-pop
CPPFLAGS	= -DCPU_P6
SRC_C		= p6.c
MODE		= tiny

include $(L4DIR)/mk/prog.mk
