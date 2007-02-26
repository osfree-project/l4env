SYSTEMS         := x86-l4v2 #x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = testovlwindow
DEFAULT_RELOC    = 0x00980000

SRC_C            = main.c
LIBS             = -ll4ovlwindow
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/window_test/common

include $(L4DIR)/mk/prog.mk
