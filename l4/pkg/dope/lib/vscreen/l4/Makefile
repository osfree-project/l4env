SYSTEMS       := x86-l4v2 amd64-l4v2 arm-l4v2
PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
TARGET         = libvscreen.a
SRC_C          = vscreen.c util.c
CLIENTIDL      = vscr.idl
PRIVATE_INCDIR = $(PKGDIR)/include

vpath util.c $(PKGDIR)/lib/vscreen/common

include $(L4DIR)/mk/lib.mk

