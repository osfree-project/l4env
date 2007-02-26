# Makefile for omega0 irqmeter example

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 x86-l4x0
TARGET	= irqmeter
SRC_C	= main.c
LIBS	= -lomega0

DEFAULT_RELOC = 0x000f00000

include $(L4DIR)/mk/prog.mk
