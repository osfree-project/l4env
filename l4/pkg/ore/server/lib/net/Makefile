#
# Makefile for ORe network driver lib
#

PKGDIR ?= ../../..
L4DIR ?= $(PKGDIR)/../..

TARGET = liblinuxnetdrv.a

DEFINES = -D__KERNEL__ -DKBUILD_BASENAME=$(basename $@) -DL4\
          -DDDE_LINUX -DHZ=100 -D__ORE__ #-DDE4X5_PARM=\"eth0:fdx\"

LINUX_HEADER_DIR = $(PKGDIR_ABS)/server/include/linux_headers

PRIVATE_INCDIR = $(LINUX_HEADER_DIR) \
                 $(DDE_INCDIR) $(LINUX24_INCDIR) \
                 $(PKGDIR_ABS)/server/include

WARNINGS ?= -Wall -Wno-unused

-include $(PKGDIR_OBJ)/server/Makeconf.bid.local
.general.d: $(PKGDIR_OBJ)/server/Makeconf.bid.local

src-$(CONFIG_ORE_RHINE) += via-rhine.c
src-$(CONFIG_ORE_PCNET32) += pcnet32.c
src-$(CONFIG_ORE_3C59X) += 3c59x.c
src-$(CONFIG_ORE_EEPRO100) += eepro100.c
src-$(CONFIG_ORE_UX_ONLY) += ux.c
src-$(CONFIG_ORE_TULIP) += $(addprefix tulip/, 21142.c interrupt.c  pnic.c \
                             tulip_core.c eeprom.c media.c pnic2.c timer.c)
src-$(CONFIG_ORE_NE2000) += ne2k-pci.c 8390.c

#FIXME Intel PRO/1000
src-$(CONFIG_ORE_PRO1000) += $(addprefix e1000/, e1000_main.c e1000_hw.c \
                               e1000_ethtool.c e1000_param.c)
src-$(CONFIG_ORE_TIGON3) += tg3.c

ifneq ($(CONFIG_ORE_TIGON3)$(CONFIG_ORE_PRO1000)$(CONFIG_ORE_RHINE)$(CONFIG_ORE_PCNET32),)
 ethtool-y += ethtool.c
endif
ifneq ($(CONFIG_ORE_EEPRO100)$(CONFIG_ORE_TULIP)$(CONFIG_ORE_PRO1000)$(CONFIG_ORE_RHINE)$(CONFIG_3C59X)$(CONFIG_ORE_PCNET32),)
 mii-y += mii.c
endif

SRC_C = loopback.c $(src-y) $(ethtool-y) $(mii-y)

include $(L4DIR)/mk/lib.mk
