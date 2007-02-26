PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME)-server.a
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

SRC_C		= desc_alloc.c dataspace.c clients.c
SERVERIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= $(SRC_DIR)/../include

include $(L4DIR)/mk/lib.mk
