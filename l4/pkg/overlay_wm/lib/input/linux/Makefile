PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libovlinput.a
SYSTEMS        = x86-linux
MODE           = host

SRC_C          = init.c input.c freeport.c
DEFINES       += -DSOCKETAPI -DDICE_DEFAULT_PORT=htons\(\(int\)dice_server_param\)
CLIENTIDL      = overlay.idl
SERVERIDL      = input_listener.idl

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/input/include \
                 $(DICE_INCDIR)

vpath input.c $(PKGDIR)/lib/input/common

include $(L4DIR)/mk/lib.mk
