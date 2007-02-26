# directories we need to know
PKGDIR ?=	..
L4DIR ?=	$(PKGDIR)/../../..

# source files
SRC_C =		clientlib.c
CLIENTIDL =	vsync.idl

PRIVATE_INCDIR = $(PKGDIR)/../include $(PKGDIR)/../config $(PKGDIR)/include

# target
TARGET = 	libvtsync_client.a
SYSTEMS =	x86-l4v2

# include lib role
include $(L4DIR)/mk/lib.mk
