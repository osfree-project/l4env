SYSTEMS         := x86-l4v2 #x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
MODE             = l4linux

TARGET           = testovlscreen

SRC_C            = main.c emul_l4rm.c
LIBS             = -ll4ovlscreen -ldm_generic -llog
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/screen_test/common

include $(L4DIR)/mk/prog.mk
