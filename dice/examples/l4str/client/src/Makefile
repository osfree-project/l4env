PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-client
DEFAULT_RELOC 	= 0x01c00000
SRC_C		= client.c
CLIENTIDL	= test.idl
SYSTEMS 	= x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

