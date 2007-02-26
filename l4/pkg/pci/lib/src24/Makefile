PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libpci24.a libpci24-single.a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= compat.c libpci-compat.c pci.c quirks.c names.c \
		  setup-res.c resource.c pci-i386.c pci-pc.c pci-irq.c
SRC_C_libpci24-single.a = lock.c
CPPFLAGS	= -D__KERNEL__ -D__L4__ -DCONFIG_PCI -DCONFIG_PCI_BIOS
PRIVATE_INCDIR	= ../include .

include $(L4DIR)/mk/lib.mk

WARNINGS	+= -Wno-unused

names.o: devlist.h classlist.h

devlist.h classlist.h: pci.ids gen-devlist
	@$(GEN_MESSAGE)
	$(VERBOSE)./gen-devlist < $<

gen-devlist: gen-devlist.c Makefile
	@$(BUILD_MESSAGE)
	$(VERBOSE)$(CC) -o $@ $<

clean::
	$(VERBOSE)$(RM) devlist.h classlist.h gen-devlist

vpath pci.ids $(PKGDIR)/lib/src24
