PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = testovlinput
SYSTEMS          = x86-linux
MODE             = host
SRC_C            = main.c
LIBS             = -lovlinput -lpthread
PRIVATE_INCDIR   = $(PKGDIR)/include

vpath main.c $(PKGDIR)/examples/input_test/common

include $(L4DIR)/mk/prog.mk
