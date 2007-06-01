#
# $Id$
#
# Makefile for the demo of the names server
#

SYSTEMS := x86-l4v2 arm-l4v2
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C			= demo.c
MODE			= sigma0

TARGET			= names_demo
DEFAULT_RELOC_x86	= 0x00e00000
DEFAULT_RELOC_arm	= 0x001a0000

include $(L4DIR)/mk/prog.mk
