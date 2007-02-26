SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = vtexttest
LIBS             = -ll4dope -lvscreen -nostdlib
DEFAULT_RELOC    = 0x00c00000

vpath % $(PKGDIR)/examples/vtexttest/common

PRIVATE_INCDIR   = $(PKGDIR)/examples/vtexttest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include

SRC_C            = vtexttest.c startup.c

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_OSKIT),y)
LIBS		+= -loskit_freebsd_m
endif
