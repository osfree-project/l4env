#
# $Id$
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET = logex_s1 logex_s2
DEFAULT_RELOC_logex_s1=0x00980000
DEFAULT_RELOC_logex_s2=0x00950000
MODE    = sigma0
SRC_C   = l4.c
vpath l4.c $(PKGDIR)/examples/threads

include $(L4DIR)/mk/prog.mk
