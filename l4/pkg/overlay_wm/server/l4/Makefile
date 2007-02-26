SYSTEMS         := x86-l4v2
PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = l4ovlwm
LIBS             = -ll4dope -lvscreen 
DEFAULT_RELOC    = 0x00b30000

PRIVATE_INCDIR   = $(OBJ_BASE)/include/l4/dope \
                   $(PKGDIR_ABS)/include \
                   $(PKGDIR_ABS)/server/include

SERVERIDL        = overlay.idl
CLIENTIDL        = input_listener.idl window_listener.idl

SRC_C            = main.c thread.c screen.c input.c \
                   window.c startup.c serverloop.c

vpath % $(PKGDIR_ABS)/server/common

include $(L4DIR)/mk/prog.mk
