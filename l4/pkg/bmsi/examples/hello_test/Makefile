#
# $Id$
#
# Makefile for the demo of the names server
#

SYSTEMS := x86-l4v2 
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C			= hello_test.c
#MODE			= l4env
#MODE			= sigma0

TARGET			= hello_test
DEFAULT_RELOC_x86	= 0x00f00000

LIBS			=

include $(L4DIR)/mk/prog.mk
