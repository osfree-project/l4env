# Makefile for performance measurement counter example

PKGDIR	= ../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS = x86_686-l4v2 x86_K7-l4v2

ifeq ($(CPU),686)
CPPFLAGS	+= -DCPU_P6
endif
ifeq ($(CPU),K7)
CPPFLAGS	+= -DCPU_K7
endif
#CPPFLAGS	+= -DDEMO

TARGET		= watchdog
DEFAULT_RELOC	= 0x00A00000

SRC_C		= main.c
MODE		= sigma0

include $(L4DIR)/mk/prog.mk
