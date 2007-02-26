PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libovlwindow.a
SYSTEMS        = x86-linux
MODE           = host

SRC_C          = init.c window.c freeport.c
DEFINES       += -DSOCKETAPI -DDICE_DEFAULT_PORT=htons\(\(int\)dice_server_param\)
CLIENTIDL      = overlay.idl
SERVERIDL      = window_listener.idl

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/window/include \
                 $(L4DIR)/tool/dice/include

vpath window.c $(PKGDIR)/lib/window/common

include $(L4DIR)/mk/lib.mk
