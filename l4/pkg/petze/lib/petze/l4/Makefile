SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = libpetze.a
PRIVATE_INCDIR   = $(PKGDIR)/include \
                   $(PKGDIR)/idl/OBJ-$(SYSTEM)
SRC_C            = libpetze.c
CLIENTIDL        = petze.idl

include $(L4DIR)/mk/lib.mk

