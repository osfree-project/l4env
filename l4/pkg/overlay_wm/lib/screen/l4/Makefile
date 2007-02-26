PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libl4ovlscreen.a
SRC_C          = init.c screen.c map.c
CLIENTIDL      = overlay.idl
PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/screen/include

vpath screen.c $(PKGDIR)/lib/screen/common

include $(L4DIR)/mk/lib.mk
