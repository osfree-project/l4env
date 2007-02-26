PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

SRC_C		= close.c clients.c transfer.c copy.c debug.c \
		  map.c map_ds.c map_vm.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= $(SRC_DIR)/../include

include $(L4DIR)/mk/lib.mk
