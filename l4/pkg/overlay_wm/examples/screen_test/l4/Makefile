SYSTEMS         := x86-l4v2 
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = testovlscreen
DEFAULT_RELOC    = 0x00b80000

SRC_C            = main.c
LIBS             = -ll4ovlscreen
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/screen_test/common

include $(L4DIR)/mk/prog.mk
