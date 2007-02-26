SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = yuvtest
LIBS             = -ll4dope -lvscreen -lm
DEFAULT_RELOC    = 0x00c00000

vpath % $(PKGDIR)/examples/yuvtest/common

PRIVATE_INCDIR   = $(PKGDIR)/examples/yuvtest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include

SRC_C            = yuvtest.c startup.c

include $(L4DIR)/mk/prog.mk
