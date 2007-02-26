SYSTEMS         := x86-l4v2 #x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
MODE             = l4linux

TARGET           = testovlinput

SRC_C            = main.c
LIBS             = -ll4ovlinput -lthread_linux -lsemaphore -lthread_linux -lloglinux
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/input_test/common

include $(L4DIR)/mk/prog.mk
