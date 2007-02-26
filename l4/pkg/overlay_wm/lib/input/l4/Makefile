PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libl4ovlinput.a
SRC_C          = init.c input.c
CLIENTIDL      = overlay.idl
SERVERIDL      = input_listener.idl

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/input/include

vpath input.c $(PKGDIR)/lib/input/common

include $(L4DIR)/mk/lib.mk
