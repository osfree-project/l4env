#
# $Id$
#
# Makefile for sources of names server
#

PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 x86-l4x0

TARGET		= $(PKGNAME)
DEFAULT_RELOC	= 0x00280000
MODE		= sigma0

PRIVATE_INCDIR	= $(PKGDIR)/server/include
LIBS		+= -llogserver

SRC_C		= names.c

include $(L4DIR)/mk/prog.mk
