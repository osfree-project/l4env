#
# $Id$
#
# Makefile for the demo2 of the events server
#

SYSTEMS := x86-l4v2
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C		= main.c
MODE		= sigma0
LIBS		= -levents

TARGET		= $(PKGNAME)_demo2
DEFAULT_RELOC	= 0x00E00000

LOGTAG	= $(addprefix ",$(addsuffix ",$(TARGET)))

include $(L4DIR)/mk/prog.mk
