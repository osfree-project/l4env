#
# $Id$
#
# Makefile for the demo1 of the event server
#

SYSTEMS := x86-l4v2 x86-l4x0
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C		= main.c
MODE		= sigma0
LIBS		= -levents


TARGET		= $(PKGNAME)_demo1
DEFAULT_RELOC	= 0x00E00000

include $(L4DIR)/mk/prog.mk
