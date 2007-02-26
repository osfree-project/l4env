SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = libl4dope.a
PRIVATE_INCDIR   = $(PKGDIR)/lib/dope/include \
                   $(PKGDIR)/server/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/idl/OBJ-$(SYSTEM)
SRC_C            = func.c init.c sync.c listener.c events.c main.c

vpath % $(PKGDIR)/lib/dope/common
vpath func.c $(PKGDIR)/lib/dope/dice
vpath events.c $(PKGDIR)/lib/dope/dice

CLIENTIDL        = dope.idl
SERVERIDL        = dopeapp.idl

include $(L4DIR)/mk/lib.mk

