PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = testovlscreen
SYSTEMS          = x86-linux
MODE             = host
SRC_C            = main.c
LIBS             = -lovlscreen
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/screen_test/common

include $(L4DIR)/mk/prog.mk
