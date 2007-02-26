PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

TARGET = liblxdrv.o.a
INSTALL_TARGET =

PRIVATE_INCDIR = $(PKGDIR)/server/lib-lxdrv/include $(PKGDIR)/server/lib-linux/include \
                 $(DDE_INCDIR) $(LINUX24_INCDIR)
DEFINES = -D__KERNEL__ -DDDE_LINUX -DHZ=100
WARNINGS = -Wall -Wstrict-prototypes -Wno-unused

CFLAGS = -ffunction-sections

include $(L4DIR)/mk/Makeconf
ifeq ($(LINUX24_AVAIL),n)
  $(error FLIPS needs Linux-2.4 sources! Enable LINUX24_AVAIL via "make config".)
endif

SRC_C = init.c

vpath %.c $(LINUX24_DIR)
vpath %.S $(LINUX24_DIR)

SRC_C += $(addprefix drivers/net/, eepro100.c mii.c)

include $(L4DIR)/mk/lib.mk
