PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

SRC_C		= open.c size.c resize.c phys_addr.c phys_addr_vm.c \
		  phys_addr_ds.c lock.c allocate.c info.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= $(SRC_DIR)/../include

include $(L4DIR)/mk/lib.mk
