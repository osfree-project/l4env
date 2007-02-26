# directories we need to know
PKGDIR ?=	..
L4DIR ?=	$(PKGDIR)/../../..

# source files
SRC_C =		clientlib.c
CLIENTIDL =	vdemuxer.idl

PRIVATE_INCDIR = $(PKGDIR)/../include $(PKGDIR_OBJ)/../config $(PKGDIR)/include

# target
TARGET = 	libvtdemuxer_client.a
SYSTEMS =	x86-l4v2

# include lib role
include $(L4DIR)/mk/lib.mk
