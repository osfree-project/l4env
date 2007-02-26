NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
SYSTEMS          = x86-l4v2
TARGET           = libpresenter_controller.a

SRC_C	= presenter_server.c presenter_view.c presenter_encapl4x.c

SERVERIDL = presenter.idl
CLIENTIDL = presenter_fprov.idl presenter_exec.idl

PRIVATE_INCDIR  = $(PKGDIR)/server/include

#DEBUG = 1

include $(L4DIR)/mk/lib.mk
