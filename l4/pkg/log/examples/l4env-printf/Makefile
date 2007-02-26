#
# $Id$
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET	= l4env_pr
DEFAULT_RELOC = 0x009b0000
SRC_C	= l4env.c

# our link libraries seem do be broken
LDFLAGS	= -u memcpy
LOGTAG	= $(addprefix ",$(addsuffix ",$(TARGET)))

DEFINES = -D__L4__ -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk
