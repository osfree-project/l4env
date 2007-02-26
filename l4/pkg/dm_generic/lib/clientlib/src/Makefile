PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

SRC_C		= close.c clients.c transfer.c copy.c debug.c \
		  map.c map_ds.c map_vm.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk
