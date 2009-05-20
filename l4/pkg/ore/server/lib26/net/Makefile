#
# Makefile for ORe network driver lib
#

PKGDIR ?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

include $(PKGDIR)/../dde/linux26/Makeconf

TARGET = liblinuxnetdrv26.o.a

WARNINGS ?= -Wall -Wno-unused 

DEFINES += -D__ORE__

-include $(PKGDIR_OBJ)/server/Makeconf.bid.local
.general.d: $(PKGDIR_OBJ)/server/Makeconf.bid.local

src-$(CONFIG_ORE_NE2000) += 8390.c ne2k-pci.c
src-$(CONFIG_ORE_EEPRO100) += e100.c
src-$(CONFIG_ORE_RTL8139) += 8139too.c
src-$(CONFIG_ORE_RTL8139CP) += 8139cp.c
src-$(CONFIG_ORE_PCNET32) += pcnet32.c
src-$(CONFIG_ORE_SKGE) += skge.c
src-$(CONFIG_ORE_SKY2) += sky2.c
src-$(CONFIG_ORE_TIGON3)  += tg3.c

PRIVATE_INCDIR +=  $(PKGDIR)/server/src \
				   $(PKGDIR_ABS)/server/include  \
				   $(PKGDIR_OBJ)/server/include

TULIP_FILES = 21142.c de2104x.c de4x5.c eeprom.c interrupt.c media.c \
			  pnic2.c pnic.c timer.c tulip_core.c 
src-$(CONFIG_ORE_TULIP) += $(addprefix tulip/, $(TULIP_FILES))
PRIVATE_INCDIR-$(CONFIG_ORE_TULIP) += $(PKGDIR)/server/lib26/net/tulip


E1000_FILES = e1000_hw.c e1000_main.c e1000_param.c e1000_ethtool.c
src-$(CONFIG_ORE_E1000) += $(addprefix e1000/, $(E1000_FILES))
PRIVATE_INCDIR-$(CONFIG_ORE_E1000) += $(PKGDIR)/server/lib26/net/e1000

E1000E_FILES = 82571.c ethtool.c ich8lan.c param.c es2lan.c \
			  lib.c netdev.c phy.c
src-$(CONFIG_ORE_E1000E) += $(addprefix e1000e/, $(E1000E_FILES))
PRIVATE_INCDIR-$(CONFIG_ORE_E1000E) += $(PKGDIR)/server/lib26/net/e1000e

PHY_FILES = $(addprefix phy/, phy.c phy_device.c mdio_bus.c)

SRC_C = loopback.c $(src-y) $(PHY_FILES)
PRIVATE_INCDIR += $(PRIVATE_INCDIR-y)

ifneq ($(L4CHECK_RUNNING),)
SRC_C += $(src-)
PRIVATE_INCDIR += $(PRIVATE_INCDIR-)
endif

CFLAGS += -ffunction-sections

include $(L4DIR)/mk/lib.mk
