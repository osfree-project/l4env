PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libl4ovlwindow.a
SRC_C          = init.c window.c
CLIENTIDL      = overlay.idl
SERVERIDL      = window_listener.idl

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/window/include

vpath window.c $(PKGDIR)/lib/window/common

include $(L4DIR)/mk/lib.mk
