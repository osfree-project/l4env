#
# $Id$
#
# Makefile for the demo of the names server
#

SYSTEMS := x86-l4v2 x86-l4x0
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C		= demo.c
MODE		= sigma0

TARGET		= names_demo
DEFAULT_RELOC	= 0x00E00000

include $(L4DIR)/mk/prog.mk
