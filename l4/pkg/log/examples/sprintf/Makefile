#
# $Id$
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET	= logex_sp
DEFAULT_RELOC = 0x00930000
MODE	= sigma0
SRC_C	= sprintf.c

LOGTAG	= $(addprefix ",$(addsuffix ",$(TARGET)))

DEFINES = -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk
