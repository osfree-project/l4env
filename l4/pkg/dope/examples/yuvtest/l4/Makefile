SYSTEMS         := x86-l4v2 amd64-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = yuvtest
LIBS             = -ll4dope -lvscreen -nostdlib
DEFAULT_RELOC    = 0x00c00000

vpath % $(PKGDIR)/examples/yuvtest/common

PRIVATE_INCDIR   = $(PKGDIR)/examples/yuvtest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include

SRC_C            = yuvtest.c startup.c

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_OSKIT),y)
LIBS		+= -loskit_freebsd_m
endif
