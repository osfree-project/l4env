PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-server
DEFAULT_RELOC	= 0x01a00000
SRC_C		= server.c
SERVERIDL	= test.idl
SYSTEMS 	= x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

