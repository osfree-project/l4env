PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-test
DEFAULT_RELOC	= 0x00980000

SERVERIDL	= ping.idl
CLIENTIDL	= ping.idl
SRC_C		= ping-template.c ping-testsuite.c

PRIVATE_INCDIR	= $(L4DIR)/tool/dice/include

SYSTEMS = x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

vpath ping-template.c $(IDL_PATH)
vpath ping-testsuite.c $(IDL_PATH)
