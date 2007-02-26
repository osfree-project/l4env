PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-test
DEFAULT_RELOC	= 0x00980000

SERVERIDL	= $(PKGNAME).idl
CLIENTIDL	= $(PKGNAME).idl
SRC_C		= $(PKGNAME)-template.c $(PKGNAME)-testsuite.c corba.c

SYSTEMS 	= x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

vpath $(PKGNAME)-template.c $(IDL_PATH)
vpath $(PKGNAME)-testsuite.c $(IDL_PATH)
