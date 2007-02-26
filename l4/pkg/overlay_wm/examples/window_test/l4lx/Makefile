SYSTEMS         := x86-l4v2 
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
MODE             = l4linux

TARGET           = testovlwindow

SRC_C            = main.c
LIBS             = -ll4ovlwindow -lthread_linux -lsemaphore -lthread_linux -lloglinux
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/window_test/common

include $(L4DIR)/mk/prog.mk
