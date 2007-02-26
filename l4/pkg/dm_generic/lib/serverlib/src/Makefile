PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME)-server.a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= desc_alloc.c dataspace.c clients.c
SERVERIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk
