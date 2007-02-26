PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

TARGET			 = libvscreen.a
SYSTEMS          = x86-linux
MODE             = host

DEFINES         += -DSOCKETAPI -DDICE_DEFAULT_PORT=htons\(\(int\)dice_server_param\)
PRIVATE_INCDIR   = $(PKGDIR)/lib/dope/include \
                   $(PKGDIR)/include \
                   $(L4DIR)/tool/dice/include

SRC_C			 =	vscreen.c 
CLIENTIDL		 =	vscr.idl

include $(L4DIR)/mk/lib.mk

