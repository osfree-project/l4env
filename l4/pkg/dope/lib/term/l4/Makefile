SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = libl4term.a
PRIVATE_INCDIR   = $(PKGDIR)/lib/dope/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/idl/OBJ-$(SYSTEM)
SRC_C            = term.c history.c

vpath term.c    $(PKGDIR)/lib/term/common
vpath history.c $(PKGDIR)/lib/term/common

include $(L4DIR)/mk/lib.mk

