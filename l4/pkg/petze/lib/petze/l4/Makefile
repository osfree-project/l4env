SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = libpetze.a
PRIVATE_INCDIR   = $(PKGDIR)/lib/petze/common \
                   $(PKGDIR)/include \
                   $(PKGDIR)/idl/OBJ-$(SYSTEM)

vpath wrapper.c $(PKGDIR)/lib/petze/common
SRC_C            = libpetze.c wrapper.c
CLIENTIDL        = petze.idl

include $(L4DIR)/mk/lib.mk
