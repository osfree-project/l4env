#
# $Id$
#

SYSTEMS:= x86-l4v2 x86-l4x0
PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET  = logex_ud
DEFAULT_RELOC = 0x00980000
MODE    = sigma0
SRC_C   = l4.c

LOGTAG  = $(addprefix ",$(addsuffix ",$(TARGET)))     

CPPFLAGS+= -D__L4__ -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk


