PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME)-server.a libfprov_ext-server.a
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

SERVERIDL	= $(PKGNAME).idl
SERVERIDL_libfprov_ext-server.a = fprov_ext.idl

include $(L4DIR)/mk/lib.mk
