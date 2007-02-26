PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = liblinuxterm.a
SYSTEMS          = x86-linux
MODE             = host

PRIVATE_INCDIR   = $(PKGDIR)/lib/dope/include \
                   $(PKGDIR)/include \

SRC_C            = term.c history.c
LIBS             = -llinuxdope -lpthread

vpath term.c    $(PKGDIR)/lib/term/common
vpath history.c $(PKGDIR)/lib/term/common

include $(L4DIR)/mk/lib.mk
