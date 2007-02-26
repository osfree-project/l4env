PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-server
DEFAULT_RELOC	= 0x00980000

SERVERIDL	= test.idl
SRC_C		= main.c

PRIVATE_INCDIR	= $(L4DIR)/tool/dice/include

SYSTEMS = x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk
