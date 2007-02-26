PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libpci.a
SYSTEMS		= x86-l4v2

SRC_C		= bios32.c compat.c libpci-compat.c pci.c
PRIVATE_INCDIR	= $(SRC_DIR)/../include
CPPFLAGS	= -D__KERNEL__ -D__L4__ -DCONFIG_PCI -DCONFIG_PCI_BIOS

include $(L4DIR)/mk/lib.mk

WARNINGS	+= -Wno-unused
