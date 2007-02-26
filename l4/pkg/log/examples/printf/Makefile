#
# $Id$
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET	= logex_pr
DEFAULT_RELOC = 0x00930000
MODE	= sigma0
SRC_C	= l4.c

LOGTAG	= $(addprefix ",$(addsuffix ",$(TARGET)))

CPPFLAGS+= -D__L4__ -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk
