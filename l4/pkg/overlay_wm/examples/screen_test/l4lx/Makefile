SYSTEMS         := x86-l4v2 
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
MODE             = l4linux

TARGET           = testovlscreen

SRC_C            = main.c
LIBS             = -ll4ovlscreen -ldm_generic -ll4env-l4lx
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/screen_test/common

include $(L4DIR)/mk/prog.mk
