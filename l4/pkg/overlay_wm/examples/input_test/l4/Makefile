SYSTEMS         := x86-l4v2 
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = testovlinput
DEFAULT_RELOC    = 0x00880000

SRC_C            = main.c
LIBS             = -ll4ovlinput
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/input_test/common

include $(L4DIR)/mk/prog.mk
