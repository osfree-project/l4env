PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = libovlscreen.a
SYSTEMS        = x86-linux
MODE           = host

SRC_C          = init.c screen.c map.c
DEFINES       += -DSOCKETAPI -DDICE_DEFAULT_PORT=htons\(\(int\)dice_server_param\)
CLIENTIDL      = overlay.idl

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/lib/screen/include \
                 $(L4DIR)/tool/dice/include

vpath screen.c $(PKGDIR)/lib/screen/common

include $(L4DIR)/mk/lib.mk
