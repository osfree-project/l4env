#
# Makefile for ORe network driver lib
#

PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

TARGET  = liblinuxnetdrv.a

DEFINES = -D__KERNEL__ -DKBUILD_BASENAME=$(basename $@) -DL4\
          -DDDE_LINUX -D__ORE__ #-DDE4X5_PARM=\"eth0:fdx\"

LINUX_HEADER_DIR = $(PKGDIR)/server/include/linux_headers

PRIVATE_INCDIR  = $(LINUX_HEADER_DIR) \
                  $(DDE_INCDIR) $(LINUX24_INCDIR) \
                  $(PKGDIR)/server/include

WARNINGS       ?= -Wall -Wno-unused

-include $(PKGDIR)/server/Makeconf.bid.local
.general.d: $(PKGDIR)/server/Makeconf.bid.local

src-$(CONFIG_ORE_EEPRO100)  += eepro100.c
src-$(CONFIG_ORE_TULIP)		+= $(addprefix tulip/, 21142.c interrupt.c	\
								pnic.c tulip_core.c	eeprom.c media.c 	\
								pnic2.c timer.c)

# TODO: add								
#src-$(CONFIG_ORE_UX_ONLY)	+= eth0ux.c	

#FIXME Intel PRO/1000
#src-$(CONFIG_ORE_PRO1000)	+= $(addprefix e1000/, e1000_main.c 		\
#								e1000_hw.c e1000_ethtool.c 				\
#								e1000_parm.c)

mii-$(CONFIG_ORE_EEPRO100)	+= mii.c
mii-$(CONFIG_ORE_TULIP)		+= mii.c
mii-$(CONFIG_ORE_PRO1000)	+= mii.c

SRC_C						= loopback.c $(src-y) $(mii-y)

include $(L4DIR)/mk/lib.mk
