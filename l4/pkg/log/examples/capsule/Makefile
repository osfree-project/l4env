#
# $Id$
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET	= logex_ca
DEFAULT_RELOC = 0x00930000
MODE	= sigma0
SRC_C	= capsule_example.c
LIBS	= -llogserver_capsule
L4LIBS	=  -static -lmain -lnames -ll4util -lrmgr

LOGTAG	= $(addprefix ",$(addsuffix ",$(TARGET)))

CPPFLAGS+= -D__L4__ -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk
