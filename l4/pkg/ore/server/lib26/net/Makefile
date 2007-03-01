#
# Makefile for ORe network driver lib
#

PKGDIR ?= ../../..
L4DIR ?= $(PKGDIR)/../..

include $(PKGDIR)/../dde/linux26/Makeconf

TARGET = liblinuxnetdrv26.o.a

WARNINGS ?= -Wall -Wno-unused 

-include $(PKGDIR_OBJ)/server/Makeconf.bid.local
.general.d: $(PKGDIR_OBJ)/server/Makeconf.bid.local

src-$(CONFIG_ORE_NE2000) += 8390.c ne2k-pci.c
src-$(CONFIG_ORE_EEPRO100) += e100.c
src-$(CONFIG_ORE_RTL8139) += 8139too.c
src-$(CONFIG_ORE_RTL8139CP) += 8139cp.c
src-$(CONFIG_ORE_PCNET32) += pcnet32.c

SRC_C = loopback.c $(src-y)

CFLAGS += -ffunction-sections

include $(L4DIR)/mk/lib.mk
