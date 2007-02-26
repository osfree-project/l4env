PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

TARGET = libore.o.a
INSTALL_TARGET =

PRIVATE_INCDIR = $(PKGDIR)/server/lib-linux/include \
                 $(DDE_INCDIR) $(LINUX24_INCDIR)
DEFINES = -D__KERNEL__ -DDDE_LINUX -DHZ=100
WARNINGS = -Wall -Wstrict-prototypes -Wno-unused

CFLAGS = -ffunction-sections

SRC_C = ore.c

include $(L4DIR)/mk/lib.mk
