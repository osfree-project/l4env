PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = ovlwm
SYSTEMS          = x86-linux
MODE             = host
DEFINES         += -DSOCKETAPI -DDICE_DEFAULT_PORT=htons\(\(int\)dice_server_param\)
LIBS             = -llinuxdope -lpthread -lm -lvscreen -ldata
SRC_C            = main.c startup.c screen.c input.c window.c \
                   thread.c serverloop.c

SERVERIDL        = overlay.idl
CLIENTIDL        = input_listener.idl window_listener.idl

PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include \
                   $(PKGDIR)/idl/OBJ-$(SYSTEM) \
                   $(PKGDIR)/server/include \
                   $(L4DIR)/pkg/dope/include \
                   $(L4DIR)/tool/dice/include

vpath %.c   $(PKGDIR)/server/common

include $(L4DIR)/mk/prog.mk
