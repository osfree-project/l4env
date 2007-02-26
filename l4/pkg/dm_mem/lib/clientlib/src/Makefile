PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= open.c size.c resize.c phys_addr.c lock.c allocate.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk
