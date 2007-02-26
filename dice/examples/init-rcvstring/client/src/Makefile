PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		 = $(PKGNAME)_client
DEFAULT_RELOC	 = 0x01a00000

SRC_C		 = client.c

CLIENTIDL	 = test.idl

PRIVATE_INCDIR	= $(L4DIR)/tool/dice/include

SYSTEMS = x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

