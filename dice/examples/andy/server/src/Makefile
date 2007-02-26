PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../../../..

TARGET		= $(PKGNAME)-test
DEFAULT_RELOC	= 0x00980000
SERVERIDL	= andy.idl
CLIENTIDL	= andy.idl
SRC_C		= andy-template.c andy-testsuite.c

SYSTEMS = x86-l4v2 x86-l4x0

include $(L4DIR)/mk/prog.mk

vpath andy-template.c $(IDL_PATH)
vpath andy-testsuite.c $(IDL_PATH)
