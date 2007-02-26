NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
SYSTEMS          = x86-l4v2 x86-l4x0
TARGET           = libcontroller.a

SRC_C	= presenter_server.c presenter_view.c

SERVERIDL = presenter.idl

PRIVATE_INCDIR  = $(PKGDIR)/server/include

include $(L4DIR)/mk/lib.mk
