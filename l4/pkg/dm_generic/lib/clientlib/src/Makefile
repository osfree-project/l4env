PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= map.c close.c clients.c transfer.c copy.c debug.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk
