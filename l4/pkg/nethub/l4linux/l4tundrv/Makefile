PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

TARGET = libl4tundrv.o.a

PRIVATE_INCDIR = $(DDE_INCDIR) .. $(LINUX24_INCDIR)
DEFINES = -D__KERNEL__ -DDDE_LINUX -DHZ=100
WARNINGS = -Wall -Wstrict-prototypes

CFLAGS = -ffunction-sections

include $(L4DIR)/mk/Makeconf
ifeq ($(LINUX24_AVAIL),n)
  $(error FLIPS needs Linux-2.4 sources! Enable LINUX24_AVAIL via "make config".)
endif

SRC_C          += l4tun.c

vpath %.c $(LINUX24_DIR)
vpath %.S $(LINUX24_DIR)

include $(L4DIR)/mk/lib.mk
