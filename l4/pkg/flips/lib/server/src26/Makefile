PKGDIR	?= ../../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

TARGET	= lib$(PKGNAME)26_l4vfs_server.a

SRC_C	= libflips_server.c

vpath %.c $(PKGDIR)/lib/server

PRIVATE_INCDIR += $(PKGDIR)/lib/server/src26

include $(L4DIR)/mk/lib.mk
