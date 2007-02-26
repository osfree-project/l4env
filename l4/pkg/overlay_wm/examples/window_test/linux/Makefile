PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = testovlwindow
SYSTEMS          = x86-linux
MODE             = host
SRC_C            = main.c
LIBS             = -lovlwindow -lpthread
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/window_test/common

include $(L4DIR)/mk/prog.mk
